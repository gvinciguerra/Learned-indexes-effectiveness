// This file is part of
// <https://github.com/gvinciguerra/Learned-indexes-effectiveness>.
// Copyright (c) 2020 Giorgio Vinciguerra.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#pragma once

#include <random>
#include <csignal>
#include <iostream>
#include "piecewise_linear_model.hpp"

constexpr auto infinite_exit_time = 1000000000ul;

template<typename Dist>
std::tuple<uint64_t, uint64_t, double, double>
simulate(Dist &gap_distribution, double epsilon, double slope, size_t ma_order, bool met_only) {
    std::random_device rd;
    thread_local std::mt19937 gen(rd());
    double x = 0;
    uint64_t strip_exit_time = infinite_exit_time;

    std::vector<double> memory(ma_order);
    std::generate(memory.begin(), memory.end(), std::bind(gap_distribution, gen));
    auto memory_sum = std::accumulate(memory.begin(), memory.end(), 0.);

    OptimalPiecewiseLinearModel<double, double> opt(epsilon, epsilon);
    opt.add_point(0, 0);

    for (uint64_t y = 1; y < infinite_exit_time; ++y) {
        auto gap = gap_distribution(gen);
        memory_sum -= memory[y % ma_order];
        x += gap + memory_sum;
        memory[y % ma_order] = gap;
        memory_sum += gap;

        if (strip_exit_time == infinite_exit_time && std::fabs(y - slope * x) > epsilon) {
            strip_exit_time = y;
            if (met_only)
                return {0, strip_exit_time, 0, 0};
        }
        if (!met_only && !opt.add_point(x, y)) {
            auto[lo, hi] = opt.get_slope_range();
            return {y, strip_exit_time, lo, hi};
        }
    }

    return {infinite_exit_time, strip_exit_time, 0, 1};
}

template<typename Dist>
std::tuple<uint64_t, uint64_t, double, double>
simulate_ar1(Dist &noise_distribution, double epsilon, double slope, double phi, bool met_only) {
    std::random_device rd;
    thread_local std::mt19937 gen(rd());
    double x = 0;
    double gap = 0;
    uint64_t strip_exit_time = infinite_exit_time;

    OptimalPiecewiseLinearModel<double, double> opt(epsilon, epsilon);
    opt.add_point(0, 0);

    for (uint64_t y = 1; y < infinite_exit_time; ++y) {
        gap = phi * gap + noise_distribution(gen);
        x += gap;
        if (strip_exit_time == infinite_exit_time && std::fabs(y - slope * x) > epsilon) {
            strip_exit_time = y;
            if (met_only)
                return {0, strip_exit_time, 0, 0};
        }
        if (!met_only && !opt.add_point(x, y)) {
            auto[lo, hi] = opt.get_slope_range();
            return {y, strip_exit_time, lo, hi};
        }
    }

    return {infinite_exit_time, strip_exit_time, 0, 1};
}

std::stringstream backup_output;

void signal_handler(int s) {
    std::cerr << backup_output.str() << std::endl;
    if (s == SIGINT) {
        std::cout << backup_output.str() << std::endl;
        exit(1);
    }
}