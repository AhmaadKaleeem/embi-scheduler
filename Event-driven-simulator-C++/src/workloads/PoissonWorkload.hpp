/**
 * @file PoissonWorkload.hpp
 * @brief Poisson arrival process: exponential inter-arrival times Exp(λ).
 *
 * Models memoryless arrivals — the most common assumption in queueing theory.
 * Inter-arrival times are exponentially distributed.
 *
 * Mean:     1 / λ
 * Variance: 1 / λ²
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#pragma once

#include "workloads/BaseWorkload.hpp"
#include "utils/Random.hpp"

namespace embi {

/**
 * @class PoissonWorkload
 * @brief Exponential inter-arrival generator (Poisson process).
 *
 * @par Example
 * @code
 * embi::PoissonWorkload w(42, 0.5);  // λ = 0.5 arrivals/tick
 * double inter = w.next();            // Exp(0.5), mean = 2 ticks
 * @endcode
 */
class PoissonWorkload final : public BaseWorkload {
public:
    /**
     * @brief Constructs a PoissonWorkload.
     * @param seed         PRNG seed.
     * @param arrival_rate λ > 0, arrivals per tick.
     * @throws std::invalid_argument if arrival_rate <= 0.
     */
    PoissonWorkload(uint64_t seed, double arrival_rate);

    double           next()              override;
    void             seed(uint64_t s)    override;
    std::string_view name() const noexcept override;
    double           mean() const noexcept override;
    double           variance() const noexcept override;

private:
    Random rng_;
    double rate_;
};

} // namespace embi
