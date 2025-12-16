# Exam

Author: [Bykov Danylo](https://github.com/DanyaBykov)

## Prerequisites

- g++
- Cmake
- python3

## Documentation

### Overwiev

This framework is an API for my own custom MPI.

It has two different options of comunication:

- Shared Memory: which uses standard POSIX shared memory, mmap and pthread synchronization
- Network Sockets: Simulates distributed computing using local sockets.

### Compilation

Go into the directory with this repo.
```
mkdir build
cd build
cmake ..
make
```

### Usage

To use this program use the run.py from the build directory

To run integral use "integral 1 ../cfg_files/func1.cfg" parameter or other config and function.
To run with shm use --mode shm

Sockets:
```
python3 ../run.py -n 4 ./mpi_example integral 1 ../cfg_files/func1.cfg
```

Shared memory:
```
python3 ../run.py -n 4 --mode shm ./mpi_example integral 1 ../cfg_files/func1.cfg
```

Consumer-producer
```
python3 ../run.py -n 4  ./mpi_example simple
```

Passing the number around in a circle
```
python3 ../run.py -n 4  ./mpi_example ring
```

### API

- MyMPI(int &argc, char** &argv): Initializes the MPI environment.
- ~MyMPI(): destructor
- get_rank(): Returns the identifier of the current process (0 to N-1).
- get_size(): Returns the total number of processes in the communicator.
- send(const void* data, size_t data_size, int dest_rank): Sends raw bytes to a specific destination rank. Wraps the send_shm and send_bytes_socket into one interface.
- recv(void* data, size_t data_size, int src_rank): Receives raw bytes from a specific source rank. Wraps the recv_shm and recv_bytes_socket into one interface.
- barrier(): Synchronizes all processes.