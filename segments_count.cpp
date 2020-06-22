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

#include <tuple>
#include <random>
#include <chrono>
#include <iostream>
#include "args.hxx"
#include "stats.hpp"
#include "common.hpp"

template<typename Rng>
void run_experiment(Rng &gap_distribution, size_t epsilon, size_t n, size_t step, size_t iterations, size_t threads) {
    size_t progress = 0;
    auto begin = std::chrono::steady_clock::now();
    auto[mean, variance] = get_moments(gap_distribution);
    auto theoretical_slope = 1 / mean;

    std::vector<RunningStat> segments(n / step + 1);

    auto get_output = [&] {
        std::stringstream s;
        s.precision(17);
        s << "n,segments_avg,segments_std" << std::endl;
        s << 1 << "," << segments[0].mean() << "," << segments[0].standard_deviation() << std::endl;
        for (size_t i = 1; i < segments.size(); ++i)
            s << i * step << "," << segments[i].mean() << "," << segments[i].standard_deviation() << std::endl;
        return s;
    };

    std::signal(SIGINT, signal_handler);
    std::signal(SIGUSR1, signal_handler);

    std::cout.precision(17);
    std::cout << "# mean " << mean << std::endl
              << "# variance " << variance << std::endl
              << "# epsilon " << epsilon << std::endl
              << "# met constant " << mean * mean / variance << std::endl;

    #pragma omp parallel for default(none) num_threads(threads) \
            shared(gap_distribution, epsilon, n, step, iterations, threads, get_output, backup_output, \
                   segments, theoretical_slope, progress, begin, std::cerr)
    for (size_t i = 0; i < iterations; ++i) {
        std::vector<size_t> checkpoints(segments.size());
        checkpoints[0] = 1;

        std::random_device rd;
        std::mt19937 gen(rd());
        double x = 0;
        size_t c = 1;
        size_t start = 0;
        for (uint64_t j = 1; j <= n; ++j) {
            x += gap_distribution(gen);
            if (std::fabs((j - start) - theoretical_slope * x) > epsilon) {
                ++c;
                x = 0;
                start = j;
            }
            if (j % step == 0)
                checkpoints[j / step] = c;
        }

        #pragma omp critical
        for (size_t j = 0; j < segments.size(); ++j)
            segments[j].push(checkpoints[j]);

        #pragma atomic
        ++progress;
        if (iterations > 1000 && progress % (iterations / 1000) == 0) {
            auto current = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(current - begin).count();
            auto seconds_left = iterations * elapsed / progress - elapsed;
            std::stringstream stream;
            stream.precision(3);
            stream << "\33[2K\r" << 100. * progress / iterations
                   << "% (" << seconds_left / 60 << "m" << seconds_left % 60 << "s left)";
            std::cerr << stream.str() << std::flush;

            if (progress % (iterations / 100) == 0) {
                #pragma omp critical
                backup_output = get_output();
            }
        }
    }

    std::cout << get_output().str();
}

int main(int argc, char **argv) {
    args::ArgumentParser ap("Experiment the number of segments of the MET algorithm on random streams of"
                            "increasing length.", "");
    args::HelpFlag help(ap, "help", "Display this help menu", {'h', "help"});

    args::Group distributions(ap, "command");
    args::Command uniform(distributions, "uniform", "Continuous uniform (min, max)");
    args::Command pareto(distributions, "pareto", "Pareto (scale k, shape α)");
    args::Command lognormal(distributions, "lognormal", "Lognormal (µ, σ)");
    args::Command exponential(distributions, "exponential", "Exponential (rate λ)");
    args::Command gamma(distributions, "gamma", "Gamma (shape k, scale θ)");

    args::Group p(ap, "", args::Group::Validators::DontCare, args::Options::Required | args::Options::Global);
    args::PositionalList<double> parameters(p, "parameters", "The distribution parameters");

    args::Group o(ap, "options", args::Group::Validators::DontCare, args::Options::Global);
    args::ValueFlag<size_t> iters(o, "iterations", "Number of generated streams", {'i'}, size_t(1e7));
    args::ValueFlag<size_t> n(o, "n", "Maximum length of each stream", {'n'}, args::Options::Required);
    args::ValueFlag<size_t> step(o, "step", "The output contains n/step samples", {'s'}, 1);
    args::ValueFlag<size_t> threads(o, "threads", "Number of threads", {'t'}, 4);
    args::ValueFlag<size_t> epsilon(o, "epsilon", "Value of ε", {'e'}, 16);

    try {
        ap.ParseCLI(argc, argv);
    }
    catch (args::Help) {
        std::cout << ap;
        return 0;
    }
    catch (args::Error e) {
        std::cerr << e.what() << std::endl;
        std::cerr << ap;
        return 1;
    }

    auto params = parameters.Get();
    if (uniform) {
        std::uniform_real_distribution<double> d(params.at(0), params.at(1));
        run_experiment(d, epsilon.Get(), n.Get(), step.Get(), iters.Get(), threads.Get());
    } else if (pareto) {
        pareto_distribution<double> d(params.at(0), params.at(1));
        run_experiment(d, epsilon.Get(), n.Get(), step.Get(), iters.Get(), threads.Get());
    } else if (lognormal) {
        std::lognormal_distribution<double> d(params.at(0), params.at(1));
        run_experiment(d, epsilon.Get(), n.Get(), step.Get(), iters.Get(), threads.Get());
    } else if (exponential) {
        std::exponential_distribution<double> d(params.at(0));
        run_experiment(d, epsilon.Get(), n.Get(), step.Get(), iters.Get(), threads.Get());
    } else if (gamma) {
        std::gamma_distribution<double> d(params.at(0), params.at(1));
        run_experiment(d, epsilon.Get(), n.Get(), step.Get(), iters.Get(), threads.Get());
    }
}