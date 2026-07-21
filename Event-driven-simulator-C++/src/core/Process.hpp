/**
 * @file Process.hpp
 * @brief Process data model: queue state, EWMA rate estimation, and timing.
 *
 * Process is a plain data class — it holds state and provides update methods.
 * Scheduler scoring is deliberately NOT here; each Scheduler reads Process
 * state and computes its own score, following the Open/Closed Principle.
 *
 * @par EWMA Update Rules
 *
 * Arrival rate estimate:
 *   λ̂ ← (1 − α) · λ̂ + α · (1 / inter_arrival_ticks)
 *
 * Service rate estimate:
 *   μ̂ ← (1 − β) · μ̂ + β · (1 / service_ticks)
 *
 * Both estimates are initialised to the true rates provided at construction
 * (from Config), so that scheduling quality is good even before the EWMA
 * has converged.
 *
 * @par Per-job waiting time
 *
 * Each job's arrival tick is pushed into job_arrival_queue_ (a FIFO deque).
 * When a job completes service, the head of the deque is popped and the
 * waiting time = current_tick - head_tick is recorded.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>

namespace embi {

/**
 * @class Process
 * @brief Simulation process with queue, EWMA estimators, and timing statistics.
 *
 * One Process instance exists per simulated process. The EventLoop creates
 * these at startup and updates them throughout the simulation.
 *
 * All fields are public for performance (no getter/setter indirection in
 * the hot loop), following the established practice in high-performance
 * simulation research.
 *
 * @par Example
 * @code
 * embi::Process p(0, 0.5, 1.0, 0.1, 0.1);
 * p.arrival(10.0);        // tick 10: job arrives
 * p.arrival(12.0);        // tick 12: another job arrives
 * double wt = p.service(14.0);  // tick 14: first job served, returns wait = 4.0
 * @endcode
 */
class Process {
public:
    // ─── Construction ─────────────────────────────────────────────────────────

    /**
     * @brief Constructs a Process with known true rates.
     *
     * EWMA estimates are pre-initialised to the true rates so that the
     * scheduler has reasonable starting scores from tick 0.
     *
     * @param id_          Unique process identifier in [0, num_processes).
     * @param true_lambda  Ground-truth arrival rate (jobs/tick).
     * @param true_mu      Ground-truth service rate (jobs/tick).
     * @param alpha_       EWMA smoothing factor for arrival estimation.
     * @param beta_        EWMA smoothing factor for service estimation.
     * @param queue_preallocate Hint for pre-allocating the job arrival deque.
     * @complexity O(queue_preallocate)
     */
    Process(std::size_t id_,
            double      true_lambda,
            double      true_mu,
            double      alpha_,
            double      beta_,
            double      lambda_noise_stddev_ = 0.0,
            std::size_t queue_preallocate = 256);

    Process(const Process&)             = default;
    Process& operator=(const Process&)  = default;
    Process(Process&&)                  = default;
    Process& operator=(Process&&)       = default;

    // ─── Identity ─────────────────────────────────────────────────────────────

    std::size_t id{0};  ///< Unique process index.

    // ─── Queue state ──────────────────────────────────────────────────────────

    int64_t  queue_length{0};    ///< Current number of jobs waiting (or in service).
    int64_t  sync_debt{0};       ///< D_i(t): Mathematical congestion state for lock contention.
    uint64_t arrival_count{0};   ///< Total jobs arrived since creation / last reset().
    uint64_t completed_jobs{0};  ///< Total jobs completed since creation / last reset().

    // ─── Lock-holder state ────────────────────────────────────────────────────

    /**
     * @struct LockState
     * @brief Tracks whether this process currently holds a lock and how much
     *        CPU time it has accumulated toward completing its critical section.
     *
     * Hold duration is measured in CPU ticks (ticks the process is actually
     * scheduled), NOT wall-clock ticks. This faithfully implements the fluid
     * model where service occurs only when the CPU is allocated to the holder.
     */
    struct LockState {
        bool        holds_lock   = false;  ///< True while this process owns a lock.
        int         held_lock_id = -1;     ///< Lock ID, or -1 if not holding.
        double      required_cpu = 0.0;    ///< CPU ticks needed to finish critical section.
        double      elapsed_cpu  = 0.0;    ///< CPU ticks accumulated while holding.
    };

    LockState lock_state;  ///< Current lock-holding state (all zeros when not holding).

    // ─── Timing ───────────────────────────────────────────────────────────────

    double first_arrival_time{-1.0};  ///< Tick of the very first arrival (-1 = never).
    double last_arrival_time{-1.0};   ///< Tick of the most recent arrival.
    double last_service_time{-1.0};   ///< Tick of the most recent service completion.
    double total_waiting_time{0.0};   ///< Cumulative waiting time across all completed jobs.

    // ─── True (ground-truth) rates ────────────────────────────────────────────

    double true_arrival_rate{0.0};  ///< λ: ground truth (for validation, not used by scheduler).
    double true_service_rate{0.0};  ///< μ: ground truth (for validation, not used by scheduler).

    // ─── EWMA estimates ───────────────────────────────────────────────────────

    double lambda_hat{0.0};  ///< Estimated arrival rate (EWMA of 1/inter-arrival).
    double mu_hat{1.0};      ///< Estimated service rate (EWMA of 1/service_time).
    double alpha{0.1};       ///< EWMA smoothing coefficient for lambda_hat.
    double beta{0.1};        ///< EWMA smoothing coefficient for mu_hat.
    double lambda_noise_stddev{0.0}; ///< Noise injected into arrival estimate.

    // ─── Starvation tracking ──────────────────────────────────────────────────

    uint64_t ticks_since_last_service{0};  ///< Ticks elapsed without being scheduled.
    uint64_t max_starvation_ticks{0};      ///< Maximum observed starvation duration.
    uint64_t starvation_events{0};         ///< Number of times starvation > threshold.

    // ─── Event methods ────────────────────────────────────────────────────────

    /**
     * @brief Records a job arrival at the given simulation tick.
     *
     * - Increments queue_length and arrival_count.
     * - Updates the EWMA arrival rate estimate from the inter-arrival interval.
     * - Pushes the arrival tick into job_arrival_queue_ for per-job wait tracking.
     *
     * @param tick  Current simulation tick (double for sub-tick precision support).
     * @complexity O(1) amortised
     */
    void arrival(double tick);

    /**
     * @brief Records a service completion at the given simulation tick.
     *
     * - Decrements queue_length (no-op if queue empty).
     * - Updates the EWMA service rate estimate.
     * - Pops the oldest arrival from job_arrival_queue_ and accumulates waiting time.
     * - Resets ticks_since_last_service.
     *
     * @param tick  Current simulation tick.
     * @return Waiting time of the served job (tick − arrival_tick). Returns 0.0
     *         if the queue was empty (defensive; should not happen in a correct sim).
     * @complexity O(1)
     */
    double service(double tick);

    /**
     * @brief Records that this process was NOT scheduled this tick.
     *
     * Increments ticks_since_last_service and updates max_starvation_ticks.
     * Called by EventLoop for every process that was not chosen.
     *
     * @param starvation_threshold  Ticks before an event is counted as starvation.
     * @complexity O(1)
     */
    void tickIdle(uint64_t starvation_threshold = 100);

    // ─── EWMA update methods ──────────────────────────────────────────────────

    /**
     * @brief Updates the arrival rate EWMA from a raw inter-arrival interval.
     *
     * λ̂ ← (1 − α) · λ̂ + α · (1 / inter_arrival_ticks)
     *
     * @param inter_arrival_ticks  Time since the previous arrival (must be > 0).
     * @complexity O(1)
     */
    void updateArrivalEstimate(double inter_arrival_ticks);

    /**
     * @brief Updates the service rate EWMA from a raw service duration.
     *
     * μ̂ ← (1 − β) · μ̂ + β · (1 / service_ticks)
     *
     * @param service_ticks  Duration of the most recent service (must be > 0).
     * @complexity O(1)
     */
    void updateServiceEstimate(double service_ticks);

    // ─── Reset ────────────────────────────────────────────────────────────────

    /**
     * @brief Resets all mutable statistics to initial state.
     *
     * EWMA estimates are NOT reset; they are re-initialised to the true rates.
     * Use this between back-to-back experiment runs with the same Config.
     *
     * @complexity O(queue_length) for clearing job_arrival_queue_
     */
    void reset();

    // ─── Read-only computed properties ────────────────────────────────────────

    /**
     * @brief Returns the arrival time of the Head-of-Line (HOL) job.
     * @return Arrival tick, or -1.0 if queue is empty.
     */
    [[nodiscard]] double holArrivalTime() const noexcept {
        return job_arrival_queue_.empty() ? -1.0 : job_arrival_queue_.front();
    }

    /**
     * @brief Returns the average waiting time per completed job.
     * @return total_waiting_time / completed_jobs, or 0.0 if no jobs completed.
     * @complexity O(1)
     */
    [[nodiscard]] double averageWaitingTime() const noexcept;

    /**
     * @brief Returns the fraction of ticks this process has been active.
     * @param total_ticks  Total simulation ticks elapsed so far.
     * @complexity O(1)
     */
    [[nodiscard]] double cpuShare(uint64_t total_ticks) const noexcept;

private:
    /// FIFO queue of job arrival ticks for per-job wait-time tracking.
    std::deque<double> job_arrival_queue_;
};

} // namespace embi
