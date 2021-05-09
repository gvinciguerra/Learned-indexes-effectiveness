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

struct ExperimentConfig {
    size_t min_epsilon;
    size_t max_epsilon;
    size_t step;
    size_t iterations;
    size_t threads;
    bool met_only;
    size_t ma_order;
    double ar1_phi;

    ExperimentConfig(size_t min_epsilon,
                     size_t max_epsilon,
                     size_t step,
                     size_t iterations,
                     size_t threads,
                     bool met_only,
                     size_t ma_order,
                     double ar1_phi)
        : min_epsilon(min_epsilon),
          max_epsilon(max_epsilon),
          step(step),
          iterations(iterations),
          threads(threads),
          met_only(met_only),
          ma_order(ma_order ? ma_order : 1),
          ar1_phi(ar1_phi) {}
};

template<typename F>
void run_experiment(const ExperimentConfig &exp, const F &f) {
    auto begin = std::chrono::steady_clock::now();
    size_t progress = 0;
    auto n_epsilon_values = exp.max_epsilon - exp.min_epsilon + 1;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint64_t> epsilon_distribution(0, exp.max_epsilon - exp.min_epsilon);
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
        for (size_t i = 0; i < opt_exit_times.size(); i += exp.step)
            s << i + exp.min_epsilon
              << "," << opt_exit_times[i].mean() << "," << opt_exit_times[i].standard_deviation()
              << "," << opt_lo[i].mean() << "," << opt_lo[i].standard_deviation()
              << "," << opt_hi[i].mean() << "," << opt_hi[i].standard_deviation()
              << "," << mean_exit_times[i].mean() << "," << mean_exit_times[i].standard_deviation()
              << "," << mean_exit_times[i].samples() << std::endl;
        return s;
    };

    std::signal(SIGINT, signal_handler);
    std::signal(SIGUSR1, signal_handler);

    #pragma omp parallel for num_threads(exp.threads)
    for (size_t i = 0; i < exp.iterations; ++i) {
        auto eps = epsilon_distribution(gen);
        auto nearest_multiple = ((eps + exp.step / 2) / exp.step) * exp.step;
        eps = exp.min_epsilon + std::min(exp.max_epsilon - exp.min_epsilon, nearest_multiple);

        auto[opt_exit_t, exit_t, lo, hi] = f(eps);

        #pragma omp critical
        if (opt_exit_t != infinite_exit_time) {
            size_t j = eps - exp.min_epsilon;
            opt_exit_times[j].push(opt_exit_t);
            mean_exit_times[j].push(exit_t);
            opt_lo[j].push(lo);
            opt_hi[j].push(hi);
        }

        #pragma atomic
        ++progress;
        if (exp.iterations > 1000 && progress % (exp.iterations / 1000) == 0) {
            auto current = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(current - begin).count();
            auto seconds_left = exp.iterations * elapsed / progress - elapsed;
            std::stringstream stream;
            stream.precision(3);
            stream << "\33[2K\r" << 100. * progress / exp.iterations
                   << "% (" << seconds_left / 60 << "m" << seconds_left % 60 << "s left)";
            std::cerr << stream.str() << std::flush;

            if (progress % (exp.iterations / 100) == 0) {
                #pragma omp critical
                backup_output = get_output();
            }
        }
    }

    std::cout << get_output().str();
}

template<typename Dist>
void run_experiment(ExperimentConfig &exp, Dist &distribution) {
    if (exp.ar1_phi != 0) {
        auto[noise_mean, noise_variance] = get_moments(distribution);

        auto mean = noise_mean / (1 - exp.ar1_phi);
        auto variance = noise_variance / (1 - exp.ar1_phi * exp.ar1_phi);
        auto met_constant = ((1 - exp.ar1_phi) / (1 + exp.ar1_phi)) * mean * mean / variance;
        auto slope = 1 / mean;

        std::cout.precision(17);
        std::cout << "# mean " << mean << std::endl
                  << "# variance " << variance << std::endl
                  << "# autoregressive process phi " << exp.ar1_phi << std::endl
                  << "# met constant " << met_constant << std::endl;

        run_experiment(exp, [&](auto e) { return simulate_ar1(distribution, e, slope, exp.ar1_phi, exp.met_only); });
        return;
    }

    auto[mean, variance] = get_moments(distribution);
    auto slope = 1 / (mean * exp.ma_order);

    std::cout.precision(17);
    std::cout << "# mean " << mean << std::endl
              << "# variance " << variance << std::endl
              << "# moving-average process order " << exp.ma_order << std::endl
              << "# met constant " << mean * mean / variance << std::endl;

    run_experiment(exp, [&](auto e) { return simulate(distribution, e, slope, exp.ma_order, exp.met_only); });
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

    args::Group o(ap, "general options", args::Group::Validators::DontCare, args::Options::Global);
    args::ValueFlag<size_t> min_eps(o, "min_epsilon", "Minimum ε value", {'m'}, 1);
    args::ValueFlag<size_t> max_eps(o, "max_epsilon", "Maximum ε value", {'M'}, 16);
    args::ValueFlag<size_t> step(o, "step", "Advance ε by this number of places", {'s'}, 1);
    args::ValueFlag<size_t> iters(o, "iterations", "Number of generated streams", {'i'}, size_t(1e7));
    args::ValueFlag<size_t> threads(o, "threads", "Number of threads", {'t'}, 4);
    args::Flag met(o, "met", "Simulate only the MET algorithm", {"met"});

    args::Group c(ap, "options to simulate correlation", args::Group::Validators::AtMostOne, args::Options::Global);
    args::ValueFlag<size_t> ma(c, "order", "Simulate a moving-average process MA(o) with the given order o", {'o'}, 0);
    args::ValueFlag<double> ar1(c, "phi", "Simulate an autoregressive process AR(1) with the given φ param", {'a'}, 0);

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

    ExperimentConfig exp(min_eps.Get(), max_eps.Get(), step.Get(), iters.Get(), threads.Get(), met.Get(),
                         ma.Get(), ar1.Get());

    auto params = parameters.Get();
    if (uniform) {
        std::uniform_real_distribution<double> d(params.at(0), params.at(1));
        run_experiment(exp, d);
    } else if (pareto) {
        pareto_distribution<double> d(params.at(0), params.at(1));
        run_experiment(exp, d);
    } else if (lognormal) {
        std::lognormal_distribution<double> d(params.at(0), params.at(1));
        run_experiment(exp, d);
    } else if (exponential) {
        std::exponential_distribution<double> d(params.at(0));
        run_experiment(exp, d);
    } else if (gamma) {
        std::gamma_distribution<double> d(params.at(0), params.at(1));
        run_experiment(exp, d);
    }
}