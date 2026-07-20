/**
 * @file Random.cpp
 * @brief Implementation of the Random PRNG wrapper and distribution samplers.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#include "utils/Random.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <stdexcept>

namespace embi {

// ─── Construction ────────────────────────────────────────────────────────────

Random::Random(uint64_t seed) noexcept
    : seed_(seed), engine_(seed) {}

// ─── State management ────────────────────────────────────────────────────────

void Random::reseed(uint64_t seed) noexcept {
    seed_   = seed;
    engine_.seed(seed);
}

uint64_t Random::currentSeed() const noexcept {
    return seed_;
}

// ─── Uniform distributions ───────────────────────────────────────────────────

double Random::uniformReal(double lo, double hi) {
    if (lo >= hi) {
        throw std::invalid_argument(
            "Random::uniformReal: lo must be strictly less than hi");
    }
    std::uniform_real_distribution<double> dist(lo, hi);
    return dist(engine_);
}

int64_t Random::uniformInt(int64_t lo, int64_t hi) {
    if (lo > hi) {
        throw std::invalid_argument(
            "Random::uniformInt: lo must be <= hi");
    }
    std::uniform_int_distribution<int64_t> dist(lo, hi);
    return dist(engine_);
}

// ─── Bernoulli ───────────────────────────────────────────────────────────────

bool Random::bernoulli(double p) {
    if (p < 0.0 || p > 1.0) {
        throw std::invalid_argument(
            "Random::bernoulli: probability p must be in [0, 1]");
    }
    std::bernoulli_distribution dist(p);
    return dist(engine_);
}

// ─── Exponential ─────────────────────────────────────────────────────────────

double Random::exponential(double rate) {
    if (rate <= 0.0) {
        throw std::invalid_argument(
            "Random::exponential: rate must be > 0");
    }
    // std::exponential_distribution parameterised by rate (lambda)
    std::exponential_distribution<double> dist(rate);
    return dist(engine_);
}

// ─── Normal ──────────────────────────────────────────────────────────────────

double Random::normal(double mean, double stddev) {
    if (stddev <= 0.0) {
        throw std::invalid_argument(
            "Random::normal: stddev must be > 0");
    }
    std::normal_distribution<double> dist(mean, stddev);
    return dist(engine_);
}

// ─── Pareto ──────────────────────────────────────────────────────────────────

double Random::pareto(double scale, double shape) {
    if (scale <= 0.0) {
        throw std::invalid_argument(
            "Random::pareto: scale (x_m) must be > 0");
    }
    if (shape <= 0.0) {
        throw std::invalid_argument(
            "Random::pareto: shape (alpha) must be > 0");
    }
    // Inverse-CDF transform: X = scale / U^(1/shape), U ~ U(0, 1)
    // Equivalently: X = scale * exp(Exp(shape))
    std::uniform_real_distribution<double> u(0.0, 1.0);
    double sample = u(engine_);
    // Guard against degenerate u = 0
    if (sample == 0.0) sample = std::numeric_limits<double>::min();
    return scale / std::pow(sample, 1.0 / shape);
}

// ─── Poisson count ───────────────────────────────────────────────────────────

uint64_t Random::poissonCount(double mean) {
    if (mean < 0.0) {
        throw std::invalid_argument(
            "Random::poissonCount: mean must be >= 0");
    }
    if (mean == 0.0) return 0ULL;

    // For moderate mean, use std::poisson_distribution (exact).
    // For very large mean, the distribution degenerates to normal but
    // std::poisson_distribution handles it internally.
    std::poisson_distribution<uint64_t> dist(mean);
    return dist(engine_);
}

// ─── Engine access ───────────────────────────────────────────────────────────

std::mt19937_64& Random::engine() noexcept {
    return engine_;
}

} // namespace embi
