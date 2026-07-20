/**
 * @file EMBIScheduler.hpp
 * @brief EMBI scheduler: score = max(0, μ̂·(2Q + 2λ̂ − M)).
 *
 * Derived from the Lyapunov drift-plus-penalty framework. The score
 * measures the expected marginal reduction in queue backlog if process i
 * is scheduled, weighted by its estimated service rate.
 *
 * Two variants are provided via the `clip` constructor parameter:
 *   - Clipped   (default): score = max(0, μ̂·(2Q + 2λ̂ − M))
 *   - Unclipped:           score = μ̂·(2Q + 2λ̂ − M)
 *
 * The unclipped variant can choose processes with negative scores when all
 * backlogs are below the threshold M, which is useful for studying
 * stability boundary behaviour.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#pragma once

#include "schedulers/BaseScheduler.hpp"

namespace embi {

/**
 * @class EMBIScheduler
 * @brief EMBI policy: Expedited Maximum Backlog Index.
 *
 * @par Formula
 *   score_i = clip(μ̂_i · (2·Q_i + 2·λ̂_i − M))
 *
 * where clip(x) = max(0, x) for the clipped variant.
 *
 * @par Complexity  O(N) per call to choose().
 *
 * @par Example
 * @code
 * embi::EMBIScheduler sched(cfg, true);   // clipped variant
 * auto decision = sched.choose(ctx);
 * @endcode
 */
class EMBIScheduler final : public BaseScheduler {
public:
    /**
     * @brief Constructs an EMBIScheduler.
     * @param config  Simulation configuration (reads M, alpha, beta).
     * @param clip    If true (default), clips negative scores to 0.
     */
    explicit EMBIScheduler(const Config& config, bool clip = true);

    Decision         choose(const SchedulerContext& ctx) override;
    std::string_view name()  const noexcept              override;

private:
    double M_;
    bool   clip_;
};

} // namespace embi
