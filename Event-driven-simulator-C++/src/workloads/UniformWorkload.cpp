/**
 * @file UniformWorkload.cpp
 * @brief Implementation of UniformWorkload.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#include "workloads/UniformWorkload.hpp"

#include <stdexcept>

namespace embi {

UniformWorkload::UniformWorkload(uint64_t seed_val, double lo, double hi)
    : rng_(seed_val), lo_(lo), hi_(hi)
{
    if (lo <= 0.0) {
        throw std::invalid_argument("UniformWorkload: lo must be > 0");
    }
    if (lo >= hi) {
        throw std::invalid_argument("UniformWorkload: lo must be < hi");
    }
}

double UniformWorkload::next() {
    return rng_.uniformReal(lo_, hi_);
}

void UniformWorkload::seed(uint64_t s) {
    rng_.reseed(s);
}

std::string_view UniformWorkload::name() const noexcept {
    return "uniform";
}

double UniformWorkload::mean() const noexcept {
    return (lo_ + hi_) / 2.0;
}

double UniformWorkload::variance() const noexcept {
    double range = hi_ - lo_;
    return (range * range) / 12.0;
}

} // namespace embi
