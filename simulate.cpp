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
void run_experiment(Rng &gap_distribution, size_t min_epsilon, size_t max_epsilon, size_t step,
                    size_t iterations, size_t threads, bool met_only) {
    auto begin = std::chrono::steady_clock::now();

    size_t progress = 0;
    auto[mean, variance] = get_moments(gap_distribution);
    auto theoretical_slope = 1 / mean;
    auto n_epsilon_values = max_epsilon - min_epsilon + 1;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint64_t> epsilon_distribution(0, max_epsilon - min_epsilon);
    std::vector<RunningStat> opt_exit_times(n_epsilon_values);
    std::vector<RunningStat> opt_lo(n_epsilon_values);
    std::vector<RunningStat> opt_hi(n_epsilon_values);
    std::vector<RunningStat> mean_exit_times(n_epsilon_values);

    auto get_output = [&] {
        std::stringstream s;
        s.precision(17);
        s << "epsilon,"
             "opt_avg,opt_std,"
             "opt_lo_avg,opt_lo_std,"
             "opt_hi_avg,opt_hi_std,"
             "met_avg,met_std,"
             "samples" << std::endl;
        for (size_t i = 0; i < opt_exit_times.size(); i += step)
            s << i + min_epsilon
              << "," << opt_exit_times[i].mean() << "," << opt_exit_times[i].standard_deviation()
              << "," << opt_lo[i].mean() << "," << opt_lo[i].standard_deviation()
              << "," << opt_hi[i].mean() << "," << opt_hi[i].standard_deviation()
              << "," << mean_exit_times[i].mean() << "," << mean_exit_times[i].standard_deviation()
              << "," << mean_exit_times[i].samples() << std::endl;
        return s;
    };

    std::signal(SIGINT, signal_handler);
    std::signal(SIGUSR1, signal_handler);

    std::cout.precision(17);
    std::cout << "# mean " << mean << std::endl
              << "# variance " << variance << std::endl
              << "# met constant " << mean * mean / variance << std::endl;

    #pragma omp parallel for default(none) num_threads(threads) \
            shared(gap_distribution, min_epsilon, max_epsilon, step, iterations, threads, met_only, begin, progress, \
                   theoretical_slope, rd, gen, epsilon_distribution, opt_exit_times, opt_lo, opt_hi, \
                   mean_exit_times, n_epsilon_values, get_output, backup_output, std::cerr)
    for (size_t i = 0; i < iterations; ++i) {
        auto eps = epsilon_distribution(gen);
        auto nearest_multiple = ((eps + step / 2) / step) * step;
        eps = min_epsilon + std::min(max_epsilon - min_epsilon, nearest_multiple);

        if (met_only) {
            auto exit_t = simulate_met(gap_distribution, eps, theoretical_slope);
            #pragma omp critical
            mean_exit_times[eps - min_epsilon].push(exit_t);
        } else {
            auto[opt_exit_t, exit_t, lo, hi] = simulate(gap_distribution, eps, theoretical_slope);

            #pragma omp critical
            if (opt_exit_t != infinite_exit_time) {
                size_t j = eps - min_epsilon;
                opt_exit_times[j].push(opt_exit_t);
                mean_exit_times[j].push(exit_t);
                opt_lo[j].push(lo);
                opt_hi[j].push(hi);
            }
        }

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
    args::ArgumentParser ap("Simulate the exit times of two algorithms (MET, OPT) on random streams.", "");
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
    args::ValueFlag<size_t> min_epsilon(o, "min_epsilon", "Minimum ε value", {'m'}, 1);
    args::ValueFlag<size_t> max_epsilon(o, "max_epsilon", "Maximum ε value", {'M'}, 16);
    args::ValueFlag<size_t> step(o, "step", "Advance ε by this number of places", {'s'}, 1);
    args::ValueFlag<size_t> iters(o, "iterations", "Number of generated streams", {'i'}, size_t(1e7));
    args::ValueFlag<size_t> threads(o, "threads", "Number of threads", {'t'}, 4);
    args::Flag met(o, "met", "Simulate only the MET algorithm", {"met"});

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
        run_experiment(d, min_epsilon.Get(), max_epsilon.Get(), step.Get(), iters.Get(), threads.Get(), met.Get());
    } else if (pareto) {
        pareto_distribution<double> d(params.at(0), params.at(1));
        run_experiment(d, min_epsilon.Get(), max_epsilon.Get(), step.Get(), iters.Get(), threads.Get(), met.Get());
    } else if (lognormal) {
        std::lognormal_distribution<double> d(params.at(0), params.at(1));
        run_experiment(d, min_epsilon.Get(), max_epsilon.Get(), step.Get(), iters.Get(), threads.Get(), met.Get());
    } else if (exponential) {
        std::exponential_distribution<double> d(params.at(0));
        run_experiment(d, min_epsilon.Get(), max_epsilon.Get(), step.Get(), iters.Get(), threads.Get(), met.Get());
    } else if (gamma) {
        std::gamma_distribution<double> d(params.at(0), params.at(1));
        run_experiment(d, min_epsilon.Get(), max_epsilon.Get(), step.Get(), iters.Get(), threads.Get(), met.Get());
    }
}