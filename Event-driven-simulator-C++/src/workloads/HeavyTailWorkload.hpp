/**
 * @file HeavyTailWorkload.hpp
 * @brief Pareto-distributed heavy-tail inter-arrival workload.
 *
 * Models workloads where most inter-arrival times are small but
 * occasionally very large inter-arrivals occur (heavy-tail phenomenon).
 * Common in web traffic, file systems, and cloud workloads.
 *
 * Distribution: Pareto(scale = x_m, shape = α)
 *   CDF:      F(x) = 1 − (x_m / x)^α,   x ≥ x_m
 *   Mean:     α · x_m / (α − 1),         for α > 1
 *   Variance: α · x_m² / ((α−1)²(α−2)), for α > 2
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#pragma once

#include "workloads/BaseWorkload.hpp"
#include "utils/Random.hpp"

namespace embi {

/**
 * @class HeavyTailWorkload
 * @brief Pareto heavy-tail inter-arrival generator.
 *
 * @par Example
 * @code
 * embi::HeavyTailWorkload w(42, 1.0, 1.5);  // x_m=1, α=1.5
 * double inter = w.next(); // mean = 3.0 ticks
 * @endcode
 */
class HeavyTailWorkload final : public BaseWorkload {
public:
    /**
     * @brief Constructs a HeavyTailWorkload.
     * @param seed   PRNG seed.
     * @param scale  Pareto minimum value x_m > 0.
     * @param shape  Pareto shape α > 0 (tail exponent; α > 1 for finite mean).
     * @throws std::invalid_argument if scale <= 0 or shape <= 0.
     */
    HeavyTailWorkload(uint64_t seed, double scale, double shape);

    double           next()              override;
    void             seed(uint64_t s)    override;
    std::string_view name() const noexcept override;
    double           mean() const noexcept override;
    double           variance() const noexcept override;

private:
    Random rng_;
    double scale_;  // x_m
    double shape_;  // α
};

} // namespace embi
