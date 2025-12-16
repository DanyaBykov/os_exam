#include "mympi.h"
#include "integrate_parallel.h"
#include <iostream>
#include <string>
#include <vector>
#include <sstream>

void run_consumer_producer(MyMPI& mpi) {
    int rank = mpi.get_rank();
    if (rank == 0) {
        int msg = 42;
        std::cout << "[Rank 0] Sending " << msg << " to Rank 1\n";
        mpi.send_val(msg, 1);
    } else if (rank == 1) {
        int val;
        mpi.recv_val(val, 0);
        std::cout << "[Rank 1] Received " << val << "\n";
    }
}

void run_ring(MyMPI& mpi) {
    int rank = mpi.get_rank();
    int size = mpi.get_size();
    int token;
    std::stringstream ss;

    if (rank == 0) {
        token = 100;
        mpi.send_val(token, 1);
        mpi.recv_val(token, size - 1);
        ss << "Rank 0 started with 100 and got back " << token << "\n";
    } else {
        mpi.recv_val(token, rank - 1);
        ss << "Rank " << rank << " got " << token;
        token++;
        mpi.send_val(token, (rank + 1) % size);
        ss << " -> sent " << token << "\n";
    }
    std::cout << ss.str();
}

int main(int argc, char** argv) {
    try {
        MyMPI mpi(argc, argv);
        
        std::string task = "simple";
        int func_id = 1;
        std::string cfg_file = "cfg_files/func1.cfg";

        for (int i = 0; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "ring") task = "ring";
            if (arg == "integral") {
                task = "integral";
                if (i + 1 < argc) func_id = std::stoi(argv[i+1]);
                if (i + 2 < argc) cfg_file = argv[i+2];
            }
        }

        if (task == "integral") {
            if (mpi.get_rank() == 0) 
                std::cout << "Running Integral Func " << func_id << " with " << cfg_file << "\n";
            run_mpi_integral(mpi, func_id, cfg_file);
        } 
        else if (task == "ring") {
            run_ring(mpi);
        } 
        else {
            if (mpi.get_size() >= 2) run_consumer_producer(mpi);
        }
        mpi.barrier();

    } catch (const std::exception& e) {
        std::cerr << "Global Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}