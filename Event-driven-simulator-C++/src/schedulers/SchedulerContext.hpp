/**
 * @file SchedulerContext.hpp
 * @brief Context bundle passed to BaseScheduler::choose().
 *
 * SchedulerContext aggregates everything a scheduler may legitimately need
 * to make a scheduling decision — no globals, no hidden state.
 *
 * Future schedulers can use additional context fields (workload metadata,
 * global statistics) without changing the interface signature.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#pragma once

#include "core/Config.hpp"
#include "core/Process.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace embi {

/**
 * @struct OnlineSnapshot
 * @brief Lightweight snapshot of online metrics for use in SchedulerContext.
 *
 * Exported by OnlineMetrics::snapshot() and embedded in SchedulerContext
 * to avoid including the full OnlineMetrics header in every scheduler.
 */
struct OnlineSnapshot {
    double   lyapunov_v{0.0};     ///< Current Lyapunov potential V(t) = Σ Q²
    double   lyapunov_drift{0.0}; ///< ΔV = V(t) − V(t−1)
    double   throughput{0.0};     ///< Rolling jobs/tick in the last window
    double   utilization{0.0};    ///< Fraction of ticks CPU was busy
    uint64_t completed_jobs{0};   ///< Total completed jobs so far
    uint64_t tick{0};             ///< Current simulation tick
};

/**
 * @struct SchedulerContext
 * @brief All information a scheduler may need to make a decision.
 *
 * Passed by const reference to BaseScheduler::choose(). All fields are
 * read-only from the scheduler's perspective.
 *
 * @par Design note
 * Schedulers must not mutate the context or store references to it beyond
 * the lifetime of a single choose() call. This enables deterministic replay
 * and thread-safe multi-CPU scheduling extensions.
 */
struct SchedulerContext {
    /// All processes with their current queue and EWMA state.
    const std::vector<Process>& processes;

    /// The simulation tick at which this scheduling decision is made.
    double current_tick{0.0};

    /// The PID chosen in the immediately preceding tick (nullopt on tick 0).
    std::optional<std::size_t> previous_decision;

    /// Snapshot of online metrics for the current tick.
    const OnlineSnapshot& global_stats;

    /// Full simulation configuration (includes M, alpha, beta, etc.).
    const Config& config;
};

} // namespace embi
