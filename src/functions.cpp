// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
//
// Created by Daniil Bykov on 25.02.2025.
//

#include "functions.h"
#include <iostream>
#include <cmath>
#include <vector>

double function_1 (const double x1, const double x2) {
    double result = 0.002;
    for (int i = -2; i < 3; i++) {
        for (int j = -2; j < 3; j++) {
            result += 1.0/(5*(i+2) + j + 3 + pow(x1 - 16*j, 6) + pow(x2 - 16*i, 6));
        }
    }
    result = pow(result, -1);
    return result;
}

double function_2 (const double x1, const double x2) {
    constexpr double a = 20;
    constexpr double b = 0.2;
    constexpr double c = 2 * M_PI;
    double result = 0.0;
    result -= a * exp(-b * sqrt(0.5 * (pow(x1, 2) + pow(x2, 2))));
    result -= exp(0.5 * ((cos(c * x1) + cos(c * x2))));
    result += a + exp(1);
    return result;
}

double function_3 (const double x1, const double x2) {
    const int m = 5;
    const int a1[] = {1, 2, 1, 1, 5};
    const int a2[] = {4, 5, 1, 2, 4};
    const int c[] = {2, 1, 4, 7, 2};
    double res = 0.0;
    for (size_t i = 0; i < m; i++) {
        res += c[i] * std::exp(-1 * M_1_PI*(pow(x1 - a1[i], 2) + pow(x2 - a2[i], 2))) *
        std::cos(M_PI*(pow(x1 - a1[i], 2) + pow(x2 - a2[i], 2)));
    }

    return -1 * res;
}

