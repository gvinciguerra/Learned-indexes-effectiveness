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

#include <vector>
#include <chrono>
#include <fstream>
#include <sstream>
#include <cstring>
#include <algorithm>
#include "args.hxx"
#include "stats.hpp"
#include "common.hpp"

template<typename TypeOut>
std::vector<TypeOut> read_dataset_csv(const std::string &filename) {
    std::vector<TypeOut> dataset;

    try {
        std::fstream in(filename);
        in.exceptions(std::ios::failbit | std::ios::badbit);
        std::string line;

        while (in.peek() != EOF && std::getline(in, line)) {
            TypeOut value;
            std::stringstream stringstream(line);
            stringstream >> value;
            dataset.push_back(value);
        }
    }
    catch (std::ios_base::failure &e) {
        std::cerr << e.what() << std::endl;
        std::cerr << std::strerror(errno) << std::endl;
        exit(1);
    }

    return dataset;
}

template<typename TypeIn, typename TypeOut>
std::vector<TypeOut> read_data_binary(const std::string &filename, bool first_is_size = true) {
    try {
        auto openmode = std::ios::in | std::ios::binary;
        if (!first_is_size)
            openmode |= std::ios::ate;

        std::fstream in(filename, openmode);
        in.exceptions(std::ios::failbit | std::ios::badbit);

        size_t size = 0;
        if (first_is_size)
            in.read((char *) &size, sizeof(TypeIn));
        else {
            size = static_cast<size_t>(in.tellg() / sizeof(TypeIn));
            in.seekg(0);
        }

        std::vector<TypeIn> data(size);
        in.read((char *) data.data(), size * sizeof(TypeIn));
        if constexpr (std::is_same<TypeIn, TypeOut>::value)
            return data;

        return std::vector<TypeOut>(data.begin(), data.end());
    }
    catch (std::ios_base::failure &e) {
        std::cerr << e.what() << std::endl;
        std::cerr << std::strerror(errno) << std::endl;
        exit(1);
    }
}

template<typename V>
void sort_and_replace_with_gaps(V &dataset) {
    std::sort(dataset.begin(), dataset.end());
    dataset.erase(std::unique(dataset.begin(), dataset.end()), dataset.end());
    for (auto i = 0; i < dataset.size() - 1; ++i)
        dataset[i] = dataset[i + 1] - dataset[i];
    dataset.pop_back();
}

int main(int argc, char **argv) {
    args::ArgumentParser p("Simulate the OPT algorithm on real data");
    args::PositionalList<std::string> paths(p, "files", "Input files");
    args::ValueFlag<size_t> min_epsilon(p, "min_epsilon", "Minimum ε value", {'m'}, 1);
    args::ValueFlag<size_t> max_epsilon(p, "max_epsilon", "Maximum ε value", {'M'}, 16);
    args::ValueFlag<size_t> threads(p, "threads", "Number of threads", {'t'}, 4);
    args::Flag binary_files(p, "binary", "Interpret the input files as binary files rather than "
                                         "text files with numbers separated by newlines", {'b'});

    try {
        p.ParseCLI(argc, argv);
    }
    catch (args::Help) {
        std::cout << p;
        return 0;
    }
    catch (args::Error &e) {
        std::cerr << e.what() << std::endl << p;
        return 1;
    }

    std::cout << "dataset,dataset_size,epsilon,opt_avg,opt_std,samples" << std::endl;

    for (auto &&path : paths) {
        auto name = path.substr(path.find_last_of("/\\") + 1);
        std::vector<uint64_t> dataset;
        if (binary_files.Get())
            dataset = read_data_binary<uint64_t, uint64_t>(path);
        else
            dataset = read_dataset_csv<uint64_t>(path);
        sort_and_replace_with_gaps(dataset);

        #pragma omp parallel for ordered schedule(static, 1) num_threads(threads.Get())
        for (auto eps = min_epsilon.Get(); eps < max_epsilon.Get(); ++eps) {
            OptimalPiecewiseLinearModel<double, double> opt(eps, eps);
            RunningStat stat;
            uint64_t x = 0;
            for (uint64_t y = 0, start = 0; y < dataset.size(); ++y) {
                x += dataset[y];
                if (!opt.add_point(x, y)) {
                    stat.push(y - start);
                    start = y;
                }
            }

            #pragma omp ordered
            std::cout << name << "," << dataset.size() << "," << eps << "," << stat.mean() << ","
                      << stat.standard_deviation() << "," << stat.samples() << std::endl;
        }
    }

    return 0;
}