/**
 * @file UniformWorkload.hpp
 * @brief Uniform inter-arrival time workload: X ~ U(lo, hi).
 *
 * Models a deterministic-ish arrival process where jobs arrive at
 * roughly regular intervals within a configurable range. Suitable for
 * embedded and batch-processing scenarios.
 *
 * Mean:     (lo + hi) / 2
 * Variance: (hi − lo)² / 12
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#pragma once

#include "workloads/BaseWorkload.hpp"
#include "utils/Random.hpp"

namespace embi {

/**
 * @class UniformWorkload
 * @brief Continuous uniform inter-arrival generator U(lo, hi).
 *
 * @par Example
 * @code
 * embi::UniformWorkload w(42, 0.5, 2.0);
 * double inter = w.next(); // sample from U(0.5, 2.0)
 * @endcode
 */
class UniformWorkload final : public BaseWorkload {
public:
    /**
     * @brief Constructs a UniformWorkload with given parameters.
     * @param seed  PRNG seed.
     * @param lo    Lower bound of inter-arrival time (inclusive), must be > 0.
     * @param hi    Upper bound of inter-arrival time (exclusive), must be > lo.
     * @throws std::invalid_argument if lo <= 0 or lo >= hi.
     */
    UniformWorkload(uint64_t seed, double lo, double hi);

    double      next()                   override;
    void        seed(uint64_t s)         override;
    std::string_view name()   const noexcept override;
    double      mean()        const noexcept override;
    double      variance()    const noexcept override;

private:
    Random rng_;
    double lo_;
    double hi_;
};

} // namespace embi
