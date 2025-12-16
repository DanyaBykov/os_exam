#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <sys/types.h>
#include <pthread.h>

const size_t MAX_SHM_BUFFER = 4096;

class MyMPI {
public:
    MyMPI(int &argc, char** &argv);
    
    ~MyMPI();

    int get_rank() const { return rank; }
    int get_size() const { return size; }

    void send(const void* data, size_t data_size, int dest_rank);
    
    void recv(void* data, size_t data_size, int src_rank);

    template<typename T>
    void send_val(const T& val, int dest) {
        send(&val, sizeof(T), dest);
    }

    template<typename T>
    void recv_val(T& val, int src) {
        recv(&val, sizeof(T), src);
    }

    void barrier();

private:
    int rank;
    int size;
    bool use_shared_memory;

    int server_fd;
    std::vector<int> peer_sockets;
    std::vector<std::string> hosts;

    void setup_sockets();
    void cleanup_sockets();
    void send_bytes_socket(int fd, const void* data, size_t len);
    void recv_bytes_socket(int fd, void* data, size_t len);

    
    int shm_fd;
    void* shm_base_ptr;
    size_t shm_total_size;
    std::string shm_name;

    struct ShmChannel {
        pthread_mutex_t mutex;
        pthread_cond_t cond;
        bool has_data;
        size_t data_size;
        char buffer[MAX_SHM_BUFFER]; 
    };

    void setup_shm();
    void cleanup_shm();
    
    ShmChannel* get_channel(int src_rank, int dest_rank);
};