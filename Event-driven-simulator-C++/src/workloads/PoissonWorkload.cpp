/**
 * @file PoissonWorkload.cpp
 * @brief Implementation of PoissonWorkload.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#include "workloads/PoissonWorkload.hpp"

#include <stdexcept>

namespace embi {

PoissonWorkload::PoissonWorkload(uint64_t seed_val, double arrival_rate)
    : rng_(seed_val), rate_(arrival_rate)
{
    if (arrival_rate <= 0.0) {
        throw std::invalid_argument("PoissonWorkload: arrival_rate must be > 0");
    }
}

double PoissonWorkload::next() {
    // Exponential inter-arrival: Exp(rate_) → mean = 1/rate_
    return rng_.exponential(rate_);
}

void PoissonWorkload::seed(uint64_t s) {
    rng_.reseed(s);
}

std::string_view PoissonWorkload::name() const noexcept {
    return "poisson";
}

double PoissonWorkload::mean() const noexcept {
    return 1.0 / rate_;
}

double PoissonWorkload::variance() const noexcept {
    return 1.0 / (rate_ * rate_);
}

} // namespace embi
