/**
 * @file OfflineMetrics.hpp
 * @brief Post-simulation metrics computed from collected data.
 *
 * OfflineMetrics accumulates raw data during simulation (waiting times,
 * queue length snapshots, V(t) series) and computes derived statistics
 * after the simulation completes.
 *
 * Computed metrics include:
 *   - P50, P95, P99 latency from waiting-time histogram
 *   - Jain Fairness Index
 *   - Starvation analysis per process
 *   - Queue length statistics (min, max, mean, median, variance, std dev)
 *   - Time to steady state
 *   - Oscillation frequency (zero crossings of ΔV)
 *   - Score variance and decision entropy distribution
 *
 * @par Memory efficiency
 * Waiting times and V(t) are stored in histograms (not raw arrays) to
 * support 1M+ ticks × 256+ processes without exceeding memory budgets.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#pragma once

#include "core/Process.hpp"
#include "schedulers/Decision.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace embi {

/**
 * @struct QueueStats
 * @brief Per-process queue length statistics computed offline.
 */
struct QueueStats {
    double min{0.0};
    double max{0.0};
    double mean{0.0};
    double median{0.0};
    double variance{0.0};
    double std_dev{0.0};
};

/**
 * @struct OfflineReport
 * @brief Complete post-simulation statistical report.
 */
struct OfflineReport {
    // ── Latency ───────────────────────────────────────────────────────────────
    double avg_waiting_time{0.0};
    double p50_waiting_time{0.0};
    double p95_waiting_time{0.0};
    double p99_waiting_time{0.0};
    double max_waiting_time{0.0};

    // ── Turnaround ────────────────────────────────────────────────────────────
    double avg_turnaround_time{0.0};

    // ── Throughput ────────────────────────────────────────────────────────────
    double total_throughput{0.0};   ///< jobs/tick over entire simulation

    // ── Fairness ──────────────────────────────────────────────────────────────
    double jain_fairness_index{0.0};  ///< Jain's fairness index over CPU shares

    // ── Starvation ────────────────────────────────────────────────────────────
    double max_starvation_ticks{0.0};  ///< Worst starvation duration across all processes
    double avg_starvation_ticks{0.0};  ///< Mean starvation duration
    uint64_t total_starvation_events{0};

    // ── Queue statistics ──────────────────────────────────────────────────────
    QueueStats queue_stats;  ///< Aggregated across all processes

    // ── Stability ─────────────────────────────────────────────────────────────
    double time_to_steady_state{-1.0};   ///< Tick when queue variance stabilised (-1 if not)
    double oscillation_frequency{0.0};   ///< Zero crossings of ΔV per 1000 ticks
    double recovery_time_after_burst{0.0};  ///< Mean ticks to recover after burst

    // ── Scheduler ─────────────────────────────────────────────────────────────
    double avg_decision_entropy{0.0};
    double avg_score_variance{0.0};
    double context_switch_rate{0.0};  ///< Fraction of ticks with process change
};

/**
 * @class OfflineMetrics
 * @brief Accumulator for post-simulation statistics.
 *
 * @par Usage
 * @code
 * embi::OfflineMetrics om(config.num_processes, config.ticks);
 * // Inside tick loop:
 * om.recordWaitingTime(waiting_time);
 * om.recordQueueSnapshot(processes, tick);
 * om.recordDecision(decision, tick);
 * // After loop:
 * auto report = om.compute();
 * @endcode
 */
class OfflineMetrics {
public:
    /// Number of histogram bins for waiting time and queue length distributions.
    static constexpr std::size_t kHistogramBins = 10'000;

    /**
     * @brief Constructs an OfflineMetrics accumulator.
     * @param num_processes  Number of concurrent processes.
     * @param max_ticks      Total simulation ticks (for histogram sizing).
     * @complexity O(N + bins)
     */
    OfflineMetrics(std::size_t num_processes, uint64_t max_ticks);

    // ─── Data collection (inside the tick loop) ───────────────────────────────

    /**
     * @brief Records the waiting time of one completed job.
     * @param waiting_time  Wait duration in ticks (≥ 0).
     * @complexity O(1)
     */
    void recordWaitingTime(double waiting_time);

    /**
     * @brief Records per-process queue lengths for this tick.
     * @param processes  Current process vector.
     * @param tick       Current simulation tick.
     * @complexity O(N)
     */
    void recordQueueSnapshot(const std::vector<Process>& processes, uint64_t tick);

    /**
     * @brief Records diagnostic fields from a scheduling decision.
     * @param decision  The Decision struct from choose().
     * @param tick      Current simulation tick.
     * @complexity O(1)
     */
    void recordDecision(const Decision& decision, uint64_t tick);

    /**
     * @brief Records a Lyapunov drift sample.
     * @param drift  ΔV for this tick.
     * @complexity O(1)
     */
    void recordDrift(double drift);

    // ─── Offline computation (after simulation completes) ────────────────────

    /**
     * @brief Computes and returns the complete offline statistical report.
     *
     * Must be called AFTER the simulation loop finishes.
     *
     * @param processes   Final process vector (for starvation and CPU share).
     * @param total_ticks Total ticks the simulation ran.
     * @return OfflineReport with all metrics populated.
     * @complexity O(N × bins + total_ticks)
     */
    [[nodiscard]] OfflineReport compute(const std::vector<Process>& processes,
                                         uint64_t                    total_ticks) const;

    // ─── Reset ────────────────────────────────────────────────────────────────

    /**
     * @brief Resets all accumulators for multi-run experiments.
     * @complexity O(bins)
     */
    void reset();

private:
    std::size_t num_processes_;
    uint64_t    max_ticks_;

    // ── Waiting time histogram ────────────────────────────────────────────────
    std::vector<uint64_t> wait_hist_;      ///< bins over [0, max_ticks_]
    double                wait_bin_width_{1.0};
    uint64_t              total_wait_count_{0};
    double                total_wait_sum_{0.0};
    double                max_wait_seen_{0.0};

    // ── Queue length histogram (per-process combined) ─────────────────────────
    std::vector<uint64_t> queue_hist_;     ///< bins over queue length [0, some_max]
    double                queue_max_{0.0};
    double                queue_sum_{0.0};
    uint64_t              queue_samples_{0};
    std::vector<double>   queue_sum_per_proc_;
    std::vector<double>   queue_sq_per_proc_;
    std::vector<double>   queue_max_per_proc_;
    std::vector<double>   queue_min_per_proc_;

    // ── Drift samples (for oscillation frequency) ────────────────────────────
    std::vector<double> drift_samples_;  ///< sampled every 100 ticks (memory-bounded)
    static constexpr std::size_t kMaxDriftSamples = 100'000;
    uint64_t sample_stride_{1};
    uint64_t sample_counter_{0};

    // ── Decision statistics ───────────────────────────────────────────────────
    double   entropy_sum_{0.0};
    double   score_var_sum_{0.0};
    uint64_t decision_count_{0};
    uint64_t context_switches_{0};
    std::size_t last_chosen_pid_{std::size_t(-1)};

    // ── Helpers ───────────────────────────────────────────────────────────────
    [[nodiscard]] double histogramPercentile(const std::vector<uint64_t>& hist,
                                              double bin_width,
                                              uint64_t total_count,
                                              double p) const noexcept;
};

} // namespace embi
