#include "mympi.h"
#include <cstring>
#include <stdexcept>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <fstream>
#include <iostream>
#include <cerrno>
#include <pthread.h>
#include <unistd.h>

MyMPI::MyMPI(int &argc, char** &argv)
	: rank(0), size(1), use_shared_memory(false), server_fd(-1), shm_fd(-1), shm_base_ptr(nullptr), shm_total_size(0)
{
    if (argc < 3) {
        throw std::runtime_error("Not enough arguments to determine rank and confi_file");
    }
    rank = std::stoi(argv[1]);
    std::string config_file = argv[2];

    std::ifstream conf(config_file);
    if (!conf) {
        throw std::runtime_error("Failed to open config file: " + config_file);
    }
    
    int mode;
    conf >> mode;
    use_shared_memory = (mode == 0);

    conf >> size;

    if (use_shared_memory) {
        if (!(conf >> shm_name)) shm_name = "/mpi_exam_shm";
        setup_shm();
    } else {
        hosts.resize(size);
        for (int i = 0; i < size; ++i) {
            conf >> hosts[i];
        }
        setup_sockets();
    }
}

MyMPI::~MyMPI() {
    if (use_shared_memory) {
        cleanup_shm();
    } else {
        cleanup_sockets();
    }
}

void MyMPI::send(const void* data, size_t data_size, int dest_rank) {
	if (use_shared_memory) {
        ShmChannel* channel = get_channel(rank, dest_rank);
        pthread_mutex_lock(&channel->mutex);

        while (channel->has_data) {
            pthread_cond_wait(&channel->cond, &channel->mutex);
        }

        if (data_size > MAX_SHM_BUFFER) {
            pthread_mutex_unlock(&channel->mutex);
            throw std::runtime_error("Data size exceeds maximum shared memory buffer size");
        }

        std::memcpy(channel->buffer, data, data_size);
        channel->data_size = data_size;
        channel->has_data = true;

        pthread_cond_signal(&channel->cond);
        pthread_mutex_unlock(&channel->mutex); 
    } else {
        send_bytes_socket(peer_sockets[dest_rank], data, data_size);
    }
}

void MyMPI::recv(void* data, size_t data_size, int src_rank) {
    if (use_shared_memory) {
        ShmChannel* channel = get_channel(src_rank, rank);
        pthread_mutex_lock(&channel->mutex);

        while (!channel->has_data) {
            pthread_cond_wait(&channel->cond, &channel->mutex);
        }

        if (data_size < channel->data_size) {
            pthread_mutex_unlock(&channel->mutex);
            throw std::runtime_error("Provided buffer is smaller than incoming data size");
        }

        std::memcpy(data, channel->buffer, channel->data_size);
        channel->has_data = false;

        pthread_cond_signal(&channel->cond);
        pthread_mutex_unlock(&channel->mutex);
    } else {
        recv_bytes_socket(peer_sockets[src_rank], data, data_size);
    }
}

void MyMPI::setup_sockets() {
    int port = 5000 + rank;
    
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) throw std::runtime_error("Failed to create socket");

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        throw std::runtime_error("Failed to bind server socket");
    }

    if (listen(server_fd, size) < 0) {
        throw std::runtime_error("Failed to listen on server socket");
    }

    peer_sockets.resize(size, -1);

    for (int i = 0; i < rank; ++i) {
        sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int peer_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
        if (peer_fd < 0) throw std::runtime_error("Accept failed");

        int their_rank;
        recv_bytes_socket(peer_fd, &their_rank, sizeof(their_rank));
        
        peer_sockets[their_rank] = peer_fd;
    }

    for (int r = rank + 1; r < size; ++r) {
        int peer_fd = -1;
        int retries = 50;

        while (retries-- > 0) {
            peer_fd = socket(AF_INET, SOCK_STREAM, 0);
            sockaddr_in peer_addr;
            std::memset(&peer_addr, 0, sizeof(peer_addr));
            peer_addr.sin_family = AF_INET;
            peer_addr.sin_port = htons(5000 + r);
            peer_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); 

            if (connect(peer_fd, (struct sockaddr*)&peer_addr, sizeof(peer_addr)) == 0) {
                break;
            }
            
            close(peer_fd);
            usleep(100000);
        }

        if (peer_fd < 0) throw std::runtime_error("Failed to connect to rank " + std::to_string(r));

        send_bytes_socket(peer_fd, &rank, sizeof(rank));

        peer_sockets[r] = peer_fd;
    }
}

void MyMPI::cleanup_sockets() {
    for (int fd : peer_sockets) {
        if (fd >= 0) {
            close(fd);
        }
    }
    if (server_fd >= 0) {
        close(server_fd);
    }
}

void MyMPI::send_bytes_socket(int fd, const void* data, size_t len) {
    const char* ptr = static_cast<const char*>(data);
    size_t total_sent = 0;
    while (total_sent < len) {
        ssize_t sent = write(fd, ptr + total_sent, len - total_sent);
        if (sent < 0) {
            if (errno == EINTR) continue;
            throw std::runtime_error("Failed to send data over socket");
        }
        total_sent += sent;
    }
}

void MyMPI::recv_bytes_socket(int fd, void* data, size_t len) {
	char* ptr = static_cast<char*>(data);
    size_t total_received = 0;
    while (total_received < len) {
        ssize_t received = read(fd, ptr + total_received, len - total_received);
        if (received < 0) {
            if (errno == EINTR) continue;
            throw std::runtime_error("Failed to receive data over socket");
        } else if (received == 0) {
            throw std::runtime_error("Socket closed unexpectedly during receive");
        }
        total_received += received;
    }
}

void MyMPI::setup_shm() {
    struct ShmHeader { volatile int initialized; };
    size_t header_size = sizeof(ShmHeader);
    shm_total_size = header_size + size * size * sizeof(ShmChannel);

    if (rank == 0) {
        shm_fd = shm_open(shm_name.c_str(), O_CREAT | O_RDWR, 0666);
        if (shm_fd < 0) throw std::runtime_error(std::string("Rank 0 failed to create SHM: ") + strerror(errno));

        if (ftruncate(shm_fd, shm_total_size) < 0) throw std::runtime_error(std::string("ftruncate failed: ") + strerror(errno));

        shm_base_ptr = mmap(nullptr, shm_total_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
        if (shm_base_ptr == MAP_FAILED) throw std::runtime_error(std::string("Rank 0 mmap failed: ") + strerror(errno));

        ShmHeader* header = reinterpret_cast<ShmHeader*>(shm_base_ptr);
        header->initialized = 0;

        char* channels_base = static_cast<char*>(shm_base_ptr) + header_size;
        for (int i = 0; i < size * size; ++i) {
            ShmChannel* channel = reinterpret_cast<ShmChannel*>(channels_base + i * sizeof(ShmChannel));

            pthread_mutexattr_t mutex_attr;
            pthread_mutexattr_init(&mutex_attr);
            pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED);
            pthread_mutex_init(&channel->mutex, &mutex_attr);
            pthread_mutexattr_destroy(&mutex_attr);

            pthread_condattr_t cond_attr;
            pthread_condattr_init(&cond_attr);
            pthread_condattr_setpshared(&cond_attr, PTHREAD_PROCESS_SHARED);
            pthread_cond_init(&channel->cond, &cond_attr);
            pthread_condattr_destroy(&cond_attr);

            channel->has_data = false;
            channel->data_size = 0;
        }

        __sync_synchronize();
        header->initialized = 1;

    } else {
        int retries = 100; 
        while (true) {
            shm_fd = shm_open(shm_name.c_str(), O_RDWR, 0666);
            if (shm_fd >= 0) break;

            if (--retries == 0) throw std::runtime_error("Timeout waiting for SHM creation");
            usleep(100000);
        }

        shm_base_ptr = mmap(nullptr, shm_total_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
        if (shm_base_ptr == MAP_FAILED) throw std::runtime_error(std::string("Worker mmap failed: ") + strerror(errno));

        ShmHeader* header = reinterpret_cast<ShmHeader*>(shm_base_ptr);
        int waits = 200;
        while (header->initialized != 1) {
            if (--waits == 0) throw std::runtime_error("Timeout waiting for SHM initialization");
            usleep(10000);
        }
        __sync_synchronize();
    }
}

void MyMPI::cleanup_shm() {
    if (shm_base_ptr) {
        munmap(shm_base_ptr, shm_total_size);
        shm_base_ptr = nullptr;
    }
    if (shm_fd >= 0) {
        close(shm_fd);
        shm_fd = -1;
    }
    if (rank == 0) {
        shm_unlink("/mpi_exam_shm");
    }
}

MyMPI::ShmChannel* MyMPI::get_channel(int src_rank, int dest_rank) {
    if (src_rank < 0 || src_rank >= size || dest_rank < 0 || dest_rank >= size) {
        throw std::runtime_error("Invalid rank for shared memory channel");
    }
    struct ShmHeader { volatile int initialized; };
    size_t header_size = sizeof(ShmHeader);
    size_t index = src_rank * size + dest_rank;
    ShmChannel* channel = reinterpret_cast<ShmChannel*>(
        static_cast<char*>(shm_base_ptr) + header_size + index * sizeof(ShmChannel));
    return channel;
}

void MyMPI::barrier() {
    if (size <= 1) return;

    char token = 1;
    char release = 2;

    if (rank == 0) {
        for (int r = 1; r < size; ++r) {
            char tmp = 0;
            recv(&tmp, sizeof(tmp), r);
        }
        for (int r = 1; r < size; ++r) {
            send(&release, sizeof(release), r);
        }
    } else {
        send(&token, sizeof(token), 0);
        char tmp = 0;
        recv(&tmp, sizeof(tmp), 0);
    }
}

