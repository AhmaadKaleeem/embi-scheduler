/**
 * @file HybridEMBIScheduler.hpp
 * @brief Hybrid EMBI scheduler with Confidence-Gated Fallback based on Rank Stability.
 *
 * This scheduler computes the EMBI scores for all processes. It checks the gap
 * between the top two scores. If the gap is smaller than the estimated noise bound
 * `eta_max` (from Config), the signal is weak, and it falls back to MaxWeight mode.
 * If the gap is larger than `eta_max`, the signal is strong, and it uses EMBI mode.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#pragma once

#include "schedulers/BaseScheduler.hpp"

namespace embi {

/**
 * @class HybridEMBIScheduler
 * @brief Hybrid policy falling back to MaxWeight when EMBI scores lack confidence.
 *
 * @par Complexity  O(N) per call to choose().
 */
class HybridEMBIScheduler final : public BaseScheduler {
public:
    /**
     * @brief Constructs a HybridEMBIScheduler.
     * @param config  Simulation configuration (reads M, eta_max).
     */
    explicit HybridEMBIScheduler(const Config& config);

    Decision         choose(const SchedulerContext& ctx) override;
    std::string_view name()  const noexcept              override;

private:
    double M_;
    double epsilon_total_;
    double tau_constant_bound_;
};

} // namespace embi
