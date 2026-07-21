/**
 * @file OnlineMetrics.hpp
 * @brief Online (per-tick) simulation metrics with zero heap allocation.
 *
 * OnlineMetrics is updated inside the hot simulation loop after every tick.
 * All accumulators are pre-allocated at construction. No heap allocation
 * occurs during update().
 *
 * @par Tracked metrics
 * - Lyapunov potential V(t) = Σ Q_i²
 * - Lyapunov drift ΔV = V(t) − V(t−1)
 * - Rolling throughput (jobs/tick over a configurable window)
 * - CPU utilization (fraction of ticks with CPU busy)
 * - Total completed jobs
 * - Total arrived jobs
 * - Scheduler decision overhead (cumulative decision_time_ns)
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#pragma once

#include "schedulers/Decision.hpp"
#include "schedulers/SchedulerContext.hpp"  // for OnlineSnapshot
#include "core/Process.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace embi {

/**
 * @class OnlineMetrics
 * @brief Per-tick metric accumulator updated in the hot simulation loop.
 *
 * All update() calls are O(N) where N = number of processes.
 *
 * snapshot() returns an OnlineSnapshot for embedding in SchedulerContext.
 *
 * @par Example
 * @code
 * embi::OnlineMetrics om(config.num_processes, 1000);
 * // inside tick loop:
 * om.update(processes, decision, tick);
 * auto snap = om.snapshot();
 * @endcode
 */
class OnlineMetrics {
public:
    /**
     * @brief Constructs OnlineMetrics with pre-allocated internal storage.
     * @param num_processes   Number of concurrent processes.
     * @param throughput_window_size  Rolling window size for throughput estimation.
     */
    OnlineMetrics(std::size_t num_processes,
                  std::size_t throughput_window_size = 1000);

    // ─── Update ───────────────────────────────────────────────────────────────

    /**
     * @brief Updates all online metrics for the current tick.
     *
     * @param processes  Current process vector.
     * @param decision   The scheduling decision made this tick.
     * @param tick       Current simulation tick.
     * @complexity O(N)
     */
    void update(const std::vector<Process>& processes,
                const Decision&             decision,
                uint64_t                    tick);

    // ─── Queries ──────────────────────────────────────────────────────────────

    /**
     * @brief Returns the most recent Lyapunov potential V(t) = Σ Q_i².
     * @complexity O(1)
     */
    [[nodiscard]] double lyapunovV()     const noexcept;

    /**
     * @brief Returns the most recent Lyapunov drift ΔV = V(t) − V(t−1).
     * @complexity O(1)
     */
    [[nodiscard]] double lyapunovDrift() const noexcept;

    /**
     * @brief Returns the rolling throughput over the last window_size ticks.
     * @complexity O(1)
     */
    [[nodiscard]] double rollingThroughput() const noexcept;

    /**
     * @brief Returns CPU utilization in [0, 1] over all ticks observed.
     * @complexity O(1)
     */
    [[nodiscard]] double utilization() const noexcept;

    /**
     * @brief Returns total completed jobs since construction.
     * @complexity O(1)
     */
    [[nodiscard]] uint64_t completedJobs() const noexcept;

    /**
     * @brief Returns total arrived jobs since construction.
     * @complexity O(1)
     */
    [[nodiscard]] uint64_t arrivedJobs() const noexcept;

    /**
     * @brief Returns cumulative scheduler decision time in nanoseconds.
     * @complexity O(1)
     */
    [[nodiscard]] double totalDecisionTimeNs() const noexcept;

    /**
     * @brief Returns mean scheduler decision time per tick in nanoseconds.
     * @complexity O(1)
     */
    [[nodiscard]] double meanDecisionTimeNs() const noexcept;

    /**
     * @brief Returns the current tick index.
     * @complexity O(1)
     */
    [[nodiscard]] uint64_t currentTick() const noexcept;

    /**
     * @brief Exports a lightweight OnlineSnapshot for embedding in SchedulerContext.
     * @complexity O(1)
     */
    [[nodiscard]] OnlineSnapshot snapshot() const noexcept;

    /**
     * @brief Returns the fraction of ticks where the Hybrid scheduler chose EMBI mode.
     */
    [[nodiscard]] double hybridEmbiFraction() const noexcept;

    /**
     * @brief Returns the per-process queue length at the last update() call.
     * @complexity O(1)
     */
    [[nodiscard]] const std::vector<int64_t>& queueLengths() const noexcept;

    // ─── Reset ────────────────────────────────────────────────────────────────

    /**
     * @brief Resets all accumulators to zero (for multi-run experiments).
     * @complexity O(N)
     */
    void reset();

private:
    std::size_t num_processes_;
    std::size_t window_size_;

    // ── Lyapunov ──────────────────────────────────────────────────────────────
    double   prev_v_{0.0};
    double   current_v_{0.0};
    double   drift_{0.0};

    // ── Throughput rolling window (ring buffer) ────────────────────────────────
    std::vector<uint64_t> completed_in_tick_;  ///< Ring buffer of per-tick completions
    std::size_t           window_head_{0};
    uint64_t              window_sum_{0};

    // ── Counters ──────────────────────────────────────────────────────────────
    uint64_t total_completed_{0};
    uint64_t total_arrived_{0};
    uint64_t busy_ticks_{0};
    uint64_t tick_count_{0};

    uint64_t hybrid_embi_ticks_{0};
    uint64_t hybrid_mw_ticks_{0};

    // ── Scheduler overhead ────────────────────────────────────────────────────
    double total_decision_ns_{0.0};

    // ── Per-process queue snapshot ────────────────────────────────────────────
    std::vector<int64_t> queue_lengths_;
};

} // namespace embi
