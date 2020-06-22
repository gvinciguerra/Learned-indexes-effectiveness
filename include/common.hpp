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

template<typename Rng>
uint64_t simulate_opt(Rng &random_gap, double epsilon, size_t limit = infinite_exit_time) {
    std::random_device rd;
    thread_local std::mt19937 gen(rd());
    double x = 0;

    OptimalPiecewiseLinearModel<double, double> opt(epsilon, epsilon);
    opt.add_point(0, 0);

    for (uint64_t y = 1; y < infinite_exit_time; ++y) {
        x += random_gap(gen);
        if (!opt.add_point(x, y))
            return y;
    }

    return infinite_exit_time;
}

template<typename Rng>
uint64_t simulate_met(Rng &random_gap, double epsilon, double theoretical_slope, size_t limit = infinite_exit_time) {
    std::random_device rd;
    thread_local std::mt19937 gen(rd());
    double x = 0;

    for (uint64_t y = 1; y < limit; ++y) {
        x += random_gap(gen);
        if (std::fabs(y - theoretical_slope * x) > epsilon)
            return y;
    }

    return infinite_exit_time;
}

template<typename Rng>
std::tuple<uint64_t, uint64_t, double, double>
simulate(Rng &random_gap, double epsilon, double theoretical_slope) {
    std::random_device rd;
    thread_local std::mt19937 gen(rd());
    double x = 0;
    uint64_t strip_exit_time = infinite_exit_time;

    OptimalPiecewiseLinearModel<double, double> opt(epsilon, epsilon);
    opt.add_point(0, 0);

    for (uint64_t y = 1; y < infinite_exit_time; ++y) {
        x += random_gap(gen);
        if (strip_exit_time == infinite_exit_time && std::fabs(y - theoretical_slope * x) > epsilon)
            strip_exit_time = y;
        if (!opt.add_point(x, y)) {
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