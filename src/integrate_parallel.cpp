#include "integrate_parallel.h"
#include "functions.h"
#include "cfg_reader.h"
#include <iostream>
#include <vector>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <chrono>

double calculate_slice(double (*func)(double, double), 
                       double x1_start, double x1_end, 
                       double x2_start, double x2_end, 
                       double steps_x, double steps_y) {
    double result = 0.0;
    double x1_step_size = (x1_end - x1_start) / steps_x;
    double x2_step_size = (x2_end - x2_start) / steps_y;

    for (int i = 0; i < steps_x; i++) {
        for (int j = 0; j < steps_y; j++) {
            double x1 = x1_start + (i + 0.5) * x1_step_size;
            double x2 = x2_start + (j + 0.5) * x2_step_size;
            result += func(x1, x2) * x1_step_size * x2_step_size;
        }
    }
    return result;
}

bool is_converged(double current, double prev, double abs_err, double rel_err) {
    if (prev == 0.0) return false; 
    double abs_diff = std::abs(current - prev);
    double rel_diff = abs_diff / std::abs(current);
    return (abs_diff < abs_err) && (rel_diff < rel_err);
}

void run_mpi_integral(MyMPI& mpi, int func_id, const std::string& cfg_file) {
    int rank = mpi.get_rank();
    int size = mpi.get_size();

    std::vector<std::string> data = read_file(cfg_file);
    auto config = input_processing(data);

    double x1_glob_start = config["x_start"];
    double x1_glob_end   = config["x_end"];
    double x2_start      = config["y_start"];
    double x2_end        = config["y_end"];
    double steps_x_glob  = config["init_steps_x"];
    double steps_y       = config["init_steps_y"];
    int max_iter         = (int)config["max_iter"];
    double abs_err       = config["abs_err"];
    double rel_err       = config["rel_err"];

    auto func_ptr = function_1;
    if (func_id == 2) func_ptr = function_2;
    else if (func_id == 3) func_ptr = function_3;

    double prev_total_result = 0.0;
    
    auto begin_time = std::chrono::steady_clock::now();

    int stop_flag = 0;
    for (int iter = 0; iter < max_iter; ++iter) {
        
        if (iter > 0) {
            if (rank == 0) {
                for (int i = 1; i < size; ++i) mpi.send_val(stop_flag, i);
            } else {
                mpi.recv_val(stop_flag, 0);
            }
        }
        
        if (stop_flag == 1) break;

        double range_len = x1_glob_end - x1_glob_start;
        double local_chunk = range_len / size;
        
        double my_x1_start = x1_glob_start + rank * local_chunk;
        double my_x1_end   = my_x1_start + local_chunk;

        double my_steps_x = steps_x_glob / size;
        if (my_steps_x < 1.0) my_steps_x = 1.0;

        double local_res = calculate_slice(func_ptr, my_x1_start, my_x1_end, 
                                           x2_start, x2_end, my_steps_x, steps_y);

        {
            std::ostringstream oss;
            oss << "[Rank " << rank << "] Iter " << iter << ": local_res = " << std::fixed << std::setprecision(6) << local_res;
            std::cout << oss.str() << std::endl;
        }

        double total_result = 0.0;

        if (rank != 0) {
            mpi.send_val(local_res, 0);
        } else {
            total_result = local_res;
            for (int i = 1; i < size; ++i) {
                double worker_res;
                mpi.recv_val(worker_res, i);
                total_result += worker_res;
            }

            if (iter > 0 && is_converged(total_result, prev_total_result, abs_err, rel_err)) {
                std::cout << "[Master] Converged at iteration " << iter << "\n";
                stop_flag = 1; 
            } else {
                stop_flag = 0;
            }
            
            prev_total_result = total_result;
            
            if (iter == max_iter - 1 || stop_flag == 1) {
                auto end_time = std::chrono::steady_clock::now();
                std::cout << "Result: " << std::fixed << std::setprecision(6) << total_result << std::endl;
                std::cout << "Time (ms): " << std::chrono::duration_cast<std::chrono::milliseconds>(end_time - begin_time).count() << std::endl;
            }
        }

        steps_x_glob *= 2;
        steps_y *= 2;
    }
}