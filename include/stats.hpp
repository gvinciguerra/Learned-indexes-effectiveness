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

template<typename RealType=double>
class pareto_distribution {
    std::exponential_distribution<RealType> exponential_distribution;

public:
    RealType shape;
    RealType scale;
    using result_type = RealType;

    explicit pareto_distribution(RealType scale = 1, RealType shape = 1)
        : exponential_distribution(shape), scale(scale), shape(shape) {}

    template<class Generator>
    RealType operator()(Generator &g) { return scale * std::exp(exponential_distribution(g)); }

};

template<typename RealType=double>
class laplace_distribution {
    std::uniform_real_distribution<RealType> d;

public:
    RealType loc;
    RealType scale;
    using result_type = RealType;

    laplace_distribution(RealType loc, RealType scale) : d(0, 1), loc(loc), scale(scale) {
        if (scale <= RealType(0))
            throw std::invalid_argument("scale must be > 0");
    }

    template<class Generator>
    RealType operator()(Generator &g) {
        auto x = d(g);
        if (x >= 0.5)
            return loc - scale * std::log(2.0 - x - x);
        else if (x > 0.0)
            return loc + scale * std::log(x + x);
        else
            return operator()(g);
    }
};

class RunningStat {
    size_t n;
    double m_oldM;
    double m_newM;
    double m_oldS;
    double m_newS;
    double m_total;

public:
    RunningStat() : n(0) {}

    void push(double x) {
        n++;
        if (n == 1) {
            m_oldM = m_newM = x;
            m_oldS = 0.0;
            m_total = x;
        } else {
            m_newM = m_oldM + (x - m_oldM) / n;
            m_newS = m_oldS + (x - m_oldM) * (x - m_newM);
            m_oldM = m_newM;
            m_oldS = m_newS;
            m_total += x;
        }
    }

    size_t samples() const {
        return n;
    }

    double mean() const {
        return (n > 0) ? m_newM : 0.0;
    }

    double variance() const {
        return ((n > 1) ? m_newS / (n - 1) : 0.0);
    }

    double standard_deviation() const {
        return sqrt(variance());
    }

    double total() const {
        return m_total;
    }
};

template<typename Dist>
std::pair<typename Dist::result_type, typename Dist::result_type> get_moments(Dist &d) {
    using T = typename Dist::result_type;

    if constexpr (std::is_same<Dist, std::uniform_real_distribution<T>>::value) {
        T interval = d.b() - d.a();
        return {(d.a() + d.b()) / 2., interval * interval / 12.};
    }

    if constexpr (std::is_same<Dist, std::exponential_distribution<T>>::value)
        return {1. / d.lambda(), 1. / (d.lambda() * d.lambda())};

    if constexpr (std::is_same<Dist, std::normal_distribution<T>>::value)
        return {d.mean(), d.stddev() * d.stddev()};

    if constexpr (std::is_same<Dist, std::lognormal_distribution<T>>::value) {
        T v = d.s() * d.s();
        return {std::exp(d.m() + v / 2.), (std::exp(v) - 1) * std::exp(2 * d.m() + v)};
    }

    if constexpr (std::is_same<Dist, std::gamma_distribution<T>>::value)
        return {d.alpha() * d.beta(), d.alpha() * d.beta() * d.beta()};

    if constexpr (std::is_same<Dist, pareto_distribution<T>>::value) {
        T minf = -std::numeric_limits<T>::infinity();
        T m = d.shape > 1. ? d.shape * d.scale / (d.shape - 1.) : minf;
        T v = d.shape > 2. ? d.scale * d.scale * d.shape / ((d.shape - 1) * (d.shape - 1) * (d.shape - 2.)) : minf;
        return {m, v};
    }

    if constexpr (std::is_same<Dist, laplace_distribution<T>>::value)
        return {d.loc, 2. * d.scale * d.scale};

    throw std::invalid_argument("Unknown distribution");
}
