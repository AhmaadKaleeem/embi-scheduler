/**
 * @file BaseScheduler.hpp
 * @brief Abstract scheduler interface and shared scoring utilities.
 *
 * All schedulers inherit from BaseScheduler and implement exactly one
 * method: choose(const SchedulerContext&). The method must return a
 * fully populated Decision struct, including diagnostics.
 *
 * @par Scoring utilities
 * computeDecisionDiagnostics() is provided as a protected helper to
 * compute the shared fields (mean, variance, entropy, runner-up) from
 * a vector of per-process scores. All concrete schedulers call this
 * helper to avoid code duplication and to guarantee consistent diagnostics.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#pragma once

#include "schedulers/Decision.hpp"
#include "schedulers/SchedulerContext.hpp"

#include <string_view>
#include <vector>

namespace embi {

/**
 * @class BaseScheduler
 * @brief Pure abstract scheduler interface.
 *
 * @par Threading
 * Scheduler instances are NOT thread-safe. Each simulation owns exactly
 * one BaseScheduler instance.
 *
 * @par Complexity contract
 * All implementations must run in O(N) where N = number of processes.
 */
class BaseScheduler {
public:
    virtual ~BaseScheduler() = default;

    // ─── Core interface ───────────────────────────────────────────────────────

    /**
     * @brief Selects one process to be scheduled this tick.
     *
     * Must be O(N). Must return Decision::idle() if all queues are empty
     * or if no process is eligible under this scheduler's policy.
     *
     * @param ctx  Read-only context bundle (see SchedulerContext.hpp).
     * @return Fully populated Decision, including diagnostics.
     * @complexity O(N)
     */
    virtual Decision choose(const SchedulerContext& ctx) = 0;

    /**
     * @brief Returns the scheduler's human-readable name.
     * @return Name string, e.g., "embi", "maxweight", "rr", "fcfs", "cmu".
     * @complexity O(1)
     */
    [[nodiscard]] virtual std::string_view name() const noexcept = 0;

protected:
    // ─── Shared diagnostic helpers ────────────────────────────────────────────

    /**
     * @brief Populates all diagnostic fields of a Decision from a score vector.
     *
     * Computes: chosen_pid, second_best_pid, score_delta, max_score,
     * mean_score, score_variance, decision_entropy (log-sum-exp stable).
     *
     * @param scores   Per-process score vector (index = process id).
     * @param d        Decision struct to populate (chosen_pid must already be set).
     * @complexity O(N)
     */
    static void computeDecisionDiagnostics(const std::vector<double>& scores,
                                            Decision&                   d);

    /**
     * @brief Returns the index of the maximum element in scores,
     *        considering only processes with non-empty queues.
     *
     * If all queues are empty, returns 0 and sets d.valid = false.
     *
     * @param scores     Per-process scores.
     * @param processes  Process vector (for queue_length check).
     * @param d          Decision to update.
     * @return Index of the best-scoring non-empty process.
     * @complexity O(N)
     */
    static std::size_t argmaxWithQueues(const std::vector<double>&    scores,
                                         const std::vector<Process>&   processes,
                                         Decision&                      d);
};

} // namespace embi
