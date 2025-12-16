//
// Created by Daniil Bykov on 25.02.2025.
//

#ifndef INTEGRATE_PARALLEL_H
#define INTEGRATE_PARALLEL_H

#include "functions.h"
#include "cfg_reader.h"
#include <iostream>
#include <vector>
#include <map>
#include <string>
#include "mympi.h"

double integrate_function_1(double x1_start, double x1_end, double x2_start, double x2_end, double step);
double integrate_function_2(double x1_start, double x1_end, double x2_start, double x2_end, double step);
double integrate_function_3(double x1_start, double x1_end, double x2_start, double x2_end, double step);

void run_mpi_integral(MyMPI& mpi, int func_id, const std::string& cfg_file);


#endif //INTEGRATE_SERIAL_H
