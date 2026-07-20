/**
 * @file OfflineMetrics.cpp
 * @brief Post-simulation statistical analysis implementation.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#include "core/OfflineMetrics.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <numeric>

namespace embi {

// ─── Construction ────────────────────────────────────────────────────────────

OfflineMetrics::OfflineMetrics(std::size_t num_processes, uint64_t max_ticks)
    : num_processes_(num_processes)
    , max_ticks_(max_ticks)
    , wait_hist_(kHistogramBins, 0ULL)
    , queue_hist_(kHistogramBins, 0ULL)
    , queue_sum_per_proc_(num_processes, 0.0)
    , queue_sq_per_proc_(num_processes, 0.0)
    , queue_max_per_proc_(num_processes, 0.0)
    , queue_min_per_proc_(num_processes, 1.0e18)
{
    wait_bin_width_ = (max_ticks_ > 0)
                        ? static_cast<double>(max_ticks_) / static_cast<double>(kHistogramBins)
                        : 1.0;

    sample_stride_ = (max_ticks_ > kMaxDriftSamples)
                       ? (max_ticks_ / kMaxDriftSamples)
                       : 1ULL;

    drift_samples_.reserve(kMaxDriftSamples);
}

// ─── Data collection ─────────────────────────────────────────────────────────

void OfflineMetrics::recordWaitingTime(double waiting_time) {
    if (waiting_time < 0.0) waiting_time = 0.0;
    max_wait_seen_ = std::max(max_wait_seen_, waiting_time);

    // Bin the waiting time
    std::size_t bin = static_cast<std::size_t>(waiting_time / wait_bin_width_);
    if (bin >= kHistogramBins) bin = kHistogramBins - 1;

    wait_hist_[bin]++;
    total_wait_count_++;
    total_wait_sum_ += waiting_time;
}

void OfflineMetrics::recordQueueSnapshot(const std::vector<Process>& processes,
                                          uint64_t /*tick*/) {
    for (std::size_t i = 0; i < processes.size() && i < num_processes_; ++i) {
        double q = static_cast<double>(processes[i].queue_length);
        queue_sum_per_proc_[i] += q;
        queue_sq_per_proc_[i]  += q * q;
        if (q > queue_max_per_proc_[i]) queue_max_per_proc_[i] = q;
        if (q < queue_min_per_proc_[i]) queue_min_per_proc_[i] = q;

        queue_sum_  += q;
        queue_max_   = std::max(queue_max_, q);

        // Bin the queue length
        std::size_t qbin = static_cast<std::size_t>(q);
        if (qbin < kHistogramBins) {
            queue_hist_[qbin]++;
        } else {
            queue_hist_[kHistogramBins - 1]++;
        }
    }
    queue_samples_++;
}

void OfflineMetrics::recordDecision(const Decision& decision, uint64_t /*tick*/) {
    if (!decision.valid) return;

    entropy_sum_  += decision.decision_entropy;
    score_var_sum_ += decision.score_variance;
    decision_count_++;

    if (decision.chosen_pid != last_chosen_pid_ && last_chosen_pid_ != std::size_t(-1)) {
        context_switches_++;
    }
    last_chosen_pid_ = decision.chosen_pid;
}

void OfflineMetrics::recordDrift(double drift) {
    sample_counter_++;
    if (sample_counter_ % sample_stride_ == 0 &&
        drift_samples_.size() < kMaxDriftSamples) {
        drift_samples_.push_back(drift);
    }
}

// ─── Offline computation ─────────────────────────────────────────────────────

OfflineReport OfflineMetrics::compute(const std::vector<Process>& processes,
                                       uint64_t                    total_ticks) const {
    OfflineReport r;

    // ── Waiting time statistics ───────────────────────────────────────────────
    if (total_wait_count_ > 0) {
        r.avg_waiting_time = total_wait_sum_ / static_cast<double>(total_wait_count_);
        r.max_waiting_time = max_wait_seen_;
        r.p50_waiting_time = histogramPercentile(wait_hist_, wait_bin_width_, total_wait_count_, 0.50);
        r.p95_waiting_time = histogramPercentile(wait_hist_, wait_bin_width_, total_wait_count_, 0.95);
        r.p99_waiting_time = histogramPercentile(wait_hist_, wait_bin_width_, total_wait_count_, 0.99);
    }

    // ── Average turnaround ────────────────────────────────────────────────────
    // Turnaround ≈ waiting + 1 (unit service time)
    r.avg_turnaround_time = r.avg_waiting_time + 1.0;

    // ── Throughput ────────────────────────────────────────────────────────────
    uint64_t total_completed = 0;
    for (const auto& p : processes) {
        total_completed += p.completed_jobs;
    }
    r.total_throughput = (total_ticks > 0)
                           ? static_cast<double>(total_completed) / static_cast<double>(total_ticks)
                           : 0.0;

    // ── Jain Fairness Index (over CPU shares) ────────────────────────────────
    // J = (Σ x_i)² / (N · Σ x_i²),  x_i = CPU share of process i
    if (!processes.empty() && total_ticks > 0) {
        double sum_x  = 0.0;
        double sum_x2 = 0.0;
        for (const auto& p : processes) {
            double xi = static_cast<double>(p.completed_jobs) /
                        static_cast<double>(total_ticks);
            sum_x  += xi;
            sum_x2 += xi * xi;
        }
        double N = static_cast<double>(processes.size());
        if (sum_x2 > 0.0) {
            r.jain_fairness_index = (sum_x * sum_x) / (N * sum_x2);
        }
    }

    // ── Starvation ────────────────────────────────────────────────────────────
    uint64_t max_starve = 0;
    double   sum_starve = 0.0;
    uint64_t total_starve_events = 0;
    for (const auto& p : processes) {
        max_starve = std::max(max_starve, p.max_starvation_ticks);
        sum_starve += static_cast<double>(p.max_starvation_ticks);
        total_starve_events += p.starvation_events;
    }
    r.max_starvation_ticks    = static_cast<double>(max_starve);
    r.avg_starvation_ticks    = processes.empty() ? 0.0 :
                                  sum_starve / static_cast<double>(processes.size());
    r.total_starvation_events = total_starve_events;

    // ── Queue statistics (aggregated across all processes) ────────────────────
    if (queue_samples_ > 0 && !processes.empty()) {
        double n_samples = static_cast<double>(queue_samples_);
        double grand_mean = queue_sum_ / (n_samples * static_cast<double>(num_processes_));

        // Overall max and min
        double gmin = *std::min_element(queue_min_per_proc_.begin(), queue_min_per_proc_.end());
        double gmax = queue_max_;

        // Overall variance (via E[X²] - (E[X])²)
        double sum_sq_total = 0.0;
        for (std::size_t i = 0; i < num_processes_; ++i) {
            sum_sq_total += queue_sq_per_proc_[i];
        }
        double ex2      = sum_sq_total / (n_samples * static_cast<double>(num_processes_));
        double variance = ex2 - grand_mean * grand_mean;

        // Median from histogram (bin 1 = queue length 1, etc.)
        double median = histogramPercentile(queue_hist_, 1.0,
                                            queue_samples_ * num_processes_, 0.50);

        r.queue_stats = QueueStats{
            gmin,
            gmax,
            grand_mean,
            median,
            std::max(0.0, variance),
            std::sqrt(std::max(0.0, variance))
        };
    }

    // ── Stability: oscillation frequency (zero crossings of ΔV) ─────────────
    if (drift_samples_.size() >= 2) {
        uint64_t crossings = 0;
        for (std::size_t i = 1; i < drift_samples_.size(); ++i) {
            if ((drift_samples_[i - 1] >= 0.0) != (drift_samples_[i] >= 0.0)) {
                crossings++;
            }
        }
        // Normalise to crossings per 1000 ticks
        double span = static_cast<double>(total_ticks);
        r.oscillation_frequency = (span > 0.0)
            ? static_cast<double>(crossings) / span * 1000.0
            : 0.0;
    }

    // ── Time to steady state ─────────────────────────────────────────────────
    // Approximate: first tick where all per-process queue_length ≤ some threshold
    // For offline analysis, we use variance of drift samples dropping below 1.0
    // (simplified steady-state detection)
    r.time_to_steady_state = -1.0;
    if (!drift_samples_.empty()) {
        double window_var = 0.0;
        constexpr std::size_t kSteadyWindow = 100;
        for (std::size_t i = kSteadyWindow; i < drift_samples_.size(); ++i) {
            double sum  = 0.0;
            double sum2 = 0.0;
            for (std::size_t j = i - kSteadyWindow; j < i; ++j) {
                sum  += drift_samples_[j];
                sum2 += drift_samples_[j] * drift_samples_[j];
            }
            double mean_d = sum / static_cast<double>(kSteadyWindow);
            window_var = sum2 / static_cast<double>(kSteadyWindow) - mean_d * mean_d;

            if (window_var < 1.0) {
                // Estimate the actual tick
                r.time_to_steady_state =
                    static_cast<double>(i) * static_cast<double>(sample_stride_);
                break;
            }
        }
    }

    // ── Scheduler diagnostics ─────────────────────────────────────────────────
    if (decision_count_ > 0) {
        r.avg_decision_entropy = entropy_sum_  / static_cast<double>(decision_count_);
        r.avg_score_variance   = score_var_sum_ / static_cast<double>(decision_count_);
        r.context_switch_rate  = (decision_count_ > 0)
            ? static_cast<double>(context_switches_) / static_cast<double>(decision_count_)
            : 0.0;
    }

    return r;
}

// ─── Reset ───────────────────────────────────────────────────────────────────

void OfflineMetrics::reset() {
    std::fill(wait_hist_.begin(),  wait_hist_.end(),  0ULL);
    std::fill(queue_hist_.begin(), queue_hist_.end(), 0ULL);
    std::fill(queue_sum_per_proc_.begin(), queue_sum_per_proc_.end(), 0.0);
    std::fill(queue_sq_per_proc_.begin(),  queue_sq_per_proc_.end(),  0.0);
    std::fill(queue_max_per_proc_.begin(), queue_max_per_proc_.end(), 0.0);
    std::fill(queue_min_per_proc_.begin(), queue_min_per_proc_.end(), 1.0e18);

    total_wait_count_  = 0;
    total_wait_sum_    = 0.0;
    max_wait_seen_     = 0.0;
    queue_max_         = 0.0;
    queue_sum_         = 0.0;
    queue_samples_     = 0;
    entropy_sum_       = 0.0;
    score_var_sum_     = 0.0;
    decision_count_    = 0;
    context_switches_  = 0;
    last_chosen_pid_   = std::size_t(-1);
    sample_counter_    = 0;
    drift_samples_.clear();
}

// ─── Histogram percentile ────────────────────────────────────────────────────

double OfflineMetrics::histogramPercentile(const std::vector<uint64_t>& hist,
                                             double bin_width,
                                             uint64_t total_count,
                                             double p) const noexcept {
    if (total_count == 0 || hist.empty()) return 0.0;

    uint64_t target = static_cast<uint64_t>(p * static_cast<double>(total_count));
    uint64_t cumulative = 0;

    for (std::size_t bin = 0; bin < hist.size(); ++bin) {
        cumulative += hist[bin];
        if (cumulative >= target) {
            return static_cast<double>(bin) * bin_width;
        }
    }
    return static_cast<double>(hist.size() - 1) * bin_width;
}

} // namespace embi
