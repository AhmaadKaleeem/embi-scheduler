/**
 * @file HeavyTailWorkload.cpp
 * @brief Implementation of HeavyTailWorkload (Pareto).
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#include "workloads/HeavyTailWorkload.hpp"

#include <cmath>
#include <limits>
#include <stdexcept>

namespace embi {

HeavyTailWorkload::HeavyTailWorkload(uint64_t seed_val, double scale, double shape)
    : rng_(seed_val), scale_(scale), shape_(shape)
{
    if (scale <= 0.0)
        throw std::invalid_argument("HeavyTailWorkload: scale (x_m) must be > 0");
    if (shape <= 0.0)
        throw std::invalid_argument("HeavyTailWorkload: shape (alpha) must be > 0");
}

double HeavyTailWorkload::next() {
    return rng_.pareto(scale_, shape_);
}

void HeavyTailWorkload::seed(uint64_t s) {
    rng_.reseed(s);
}

std::string_view HeavyTailWorkload::name() const noexcept {
    return "heavy_tail";
}

double HeavyTailWorkload::mean() const noexcept {
    // Mean of Pareto(x_m, α) = α·x_m / (α − 1), defined for α > 1
    if (shape_ <= 1.0) {
        return std::numeric_limits<double>::infinity();
    }
    return (shape_ * scale_) / (shape_ - 1.0);
}

double HeavyTailWorkload::variance() const noexcept {
    // Var of Pareto(x_m, α) = α·x_m² / ((α−1)²·(α−2)), defined for α > 2
    if (shape_ <= 2.0) {
        return std::numeric_limits<double>::infinity();
    }
    double num = shape_ * scale_ * scale_;
    double den = (shape_ - 1.0) * (shape_ - 1.0) * (shape_ - 2.0);
    return num / den;
}

} // namespace embi
