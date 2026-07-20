/**
 * @file EventLoop.hpp
 * @brief Discrete-event simulation engine: the innermost hot loop.
 *
 * EventLoop owns the process vector, EventQueue, and next-arrival clock per
 * process. On each tick it:
 *   1. Generates Arrival events for processes whose next-arrival time has come.
 *   2. Pushes Schedule and Metrics events.
 *   3. Drains all events at the current tick from the EventQueue in type order.
 *   4. Updates online/offline metrics.
 *   5. Writes log records at the configured frequency.
 *
 * No heap allocation occurs inside run() except EventQueue internal reallocs
 * (which are bounded by the initial reservation).
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#pragma once

#include "core/Config.hpp"
#include "core/Event.hpp"
#include "core/EventQueue.hpp"
#include "core/LockManager.hpp"
#include "core/OfflineMetrics.hpp"
#include "core/OnlineMetrics.hpp"
#include "core/Process.hpp"
#include "logging/StatisticsDatabase.hpp"
#include "schedulers/BaseScheduler.hpp"
#include "schedulers/Decision.hpp"
#include "workloads/BaseWorkload.hpp"
#include "workloads/LockContentionWorkload.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

namespace embi {

/**
 * @class EventLoop
 * @brief The innermost simulation engine: tick loop with event dispatch.
 *
 * @par Ownership
 * EventLoop takes non-owning references to the workload, scheduler,
 * metrics objects, and statistics database. The Simulator owns them.
 *
 * @par Performance targets
 * - 1,000,000 ticks × 256 processes in < 60 s (Release build, single core)
 * - Zero heap allocation in the steady-state loop (after initial setup)
 */
class EventLoop {
public:
    /**
     * @brief Constructs the EventLoop and pre-allocates all internal vectors.
     *
     * @param config           Simulation configuration.
     * @param workload         Non-owning pointer to the workload generator.
     * @param scheduler        Non-owning pointer to the scheduler.
     * @param online_metrics   Non-owning pointer to the online metrics accumulator.
     * @param offline_metrics  Non-owning pointer to the offline metrics accumulator.
     * @param stats_db         Non-owning reference to the statistics database.
     * @complexity O(N) for pre-allocation.
     */
    EventLoop(const Config&       config,
              BaseWorkload*       workload,
              BaseScheduler*      scheduler,
              OnlineMetrics*      online_metrics,
              OfflineMetrics*     offline_metrics,
              StatisticsDatabase& stats_db);

    // ─── Primary interface ─────────────────────────────────────────────────────

    /**
     * @brief Runs the simulation from tick 0 to config.ticks − 1.
     *
     * After run() returns:
     *   - online_metrics contains the final snapshot.
     *   - offline_metrics has accumulated all waiting times, queue snapshots, etc.
     *   - stats_db logger has been flushed but NOT closed (caller closes it).
     *
     * @complexity O(ticks × N)
     */
    void run();

    // ─── Process access ────────────────────────────────────────────────────────

    /**
     * @brief Returns a const reference to the process vector after simulation.
     * @complexity O(1)
     */
    [[nodiscard]] const std::vector<Process>& processes() const noexcept;

private:
    // ─── Dependencies (non-owning) ────────────────────────────────────────────
    const Config&       config_;
    BaseWorkload*       workload_;
    BaseScheduler*      scheduler_;
    OnlineMetrics*      online_metrics_;
    OfflineMetrics*     offline_metrics_;
    StatisticsDatabase& stats_db_;

    // ─── State ─────────────────────────────────────────────────────────────────────────
    std::vector<Process>  processes_;           ///< All simulated processes.
    EventQueue            event_queue_;         ///< Priority queue for events.
    std::vector<double>   next_arrival_tick_;   ///< Per-process next-arrival counter.
    std::optional<std::size_t> prev_decision_;  ///< PID chosen last tick.
    Decision              last_decision_;       ///< Shared across event handlers.

    // ─── Lock-contention state (null unless workload == lock_contention) ─────────
    std::unique_ptr<LockManager>         lock_mgr_;             ///< Lock pool.
    LockContentionWorkload*              lock_workload_{nullptr}; ///< Non-owning cast.
    std::vector<double>                  next_lock_req_tick_;   ///< Per-process next lock-request timer.

    // ─── Event handlers ────────────────────────────────────────────────────────────
    void handleArrivalEvent    (const Event& e, uint64_t tick);
    void handleLockAcquireEvent(const Event& e, uint64_t tick);
    void handleLockReleaseEvent(const Event& e, uint64_t tick);
    void handleScheduleEvent   (const Event& e, uint64_t tick);
    void handleServiceEvent    (const Event& e, uint64_t tick, double& waiting_time_out);
    void handleMetricsEvent    (const Event& e, uint64_t tick, double waiting_time);

    // ─── Lock-contention helper ───────────────────────────────────────────────────
    /// Promote waiter to new owner and set up their LockState.
    void promoteLockWaiter(std::size_t lock_id, int new_owner_pid, double tick_d);

    // ─── Logging helper ────────────────────────────────────────────────────────────
    void emitLogRecords(uint64_t tick, double waiting_time);
};

} // namespace embi
