/**
 * @file OnlineMetrics.cpp
 * @brief Implementation of per-tick online metric accumulators.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#include "core/OnlineMetrics.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace embi {

// ─── Construction ────────────────────────────────────────────────────────────

OnlineMetrics::OnlineMetrics(std::size_t num_processes,
                              std::size_t throughput_window_size)
    : num_processes_(num_processes)
    , window_size_(throughput_window_size > 0 ? throughput_window_size : 1)
    , completed_in_tick_(throughput_window_size, 0ULL)
    , queue_lengths_(num_processes, 0)
{}

// ─── Update ──────────────────────────────────────────────────────────────────

void OnlineMetrics::update(const std::vector<Process>& processes,
                            const Decision&             decision,
                            uint64_t                    /*tick*/) {
    tick_count_++;

    if (decision.mode_flag == 1) {
        hybrid_mw_ticks_++;
    } else if (decision.mode_flag == 2) {
        hybrid_embi_ticks_++;
    }

    // ── Queue lengths snapshot ────────────────────────────────────────────────
    uint64_t arrived_this_tick    = 0;
    uint64_t completed_this_tick  = 0;

    double v_t = 0.0;
    for (std::size_t i = 0; i < processes.size() && i < num_processes_; ++i) {
        int64_t q         = processes[i].queue_length;
        queue_lengths_[i] = q;

        double qd = static_cast<double>(q);
        v_t += qd * qd;  // V(t) = Σ Q_i²

        // Deltas relative to last snapshot would require storing previous
        // values; instead, accumulate totals from process counters.
        // Note: processes track cumulative counts; we read them at each tick.
    }

    // ── Lyapunov ─────────────────────────────────────────────────────────────
    prev_v_    = current_v_;
    current_v_ = v_t;
    drift_     = current_v_ - prev_v_;

    // ── Throughput rolling window ─────────────────────────────────────────────
    // We track completed jobs from the Decision (valid = CPU was busy)
    uint64_t completions = decision.valid ? 1ULL : 0ULL;

    // Update ring buffer
    std::size_t ring_idx = tick_count_ % window_size_;
    window_sum_ -= completed_in_tick_[ring_idx];
    completed_in_tick_[ring_idx] = completions;
    window_sum_ += completions;

    // ── CPU utilization ───────────────────────────────────────────────────────
    if (decision.valid) {
        busy_ticks_++;
        total_completed_++;
    }

    // ── Accumulate arrived from all processes (incremental delta)
    // We cannot easily track per-tick deltas without storing the previous
    // arrival counts. Instead, we sum all processes' arrival counts each tick.
    // This is O(N) but correct. For N=256 this is ~1μs overhead per tick.
    uint64_t current_total_arrived = 0;
    for (std::size_t i = 0; i < processes.size(); ++i) {
        current_total_arrived += processes[i].arrival_count;
    }
    total_arrived_ = current_total_arrived;

    // ── Scheduler overhead ────────────────────────────────────────────────────
    total_decision_ns_ += decision.decision_time_ns;

    (void)arrived_this_tick;
    (void)completed_this_tick;
}

// ─── Queries ─────────────────────────────────────────────────────────────────

double OnlineMetrics::lyapunovV() const noexcept {
    return current_v_;
}

double OnlineMetrics::lyapunovDrift() const noexcept {
    return drift_;
}

double OnlineMetrics::rollingThroughput() const noexcept {
    if (window_size_ == 0) return 0.0;
    return static_cast<double>(window_sum_) / static_cast<double>(window_size_);
}

double OnlineMetrics::utilization() const noexcept {
    if (tick_count_ == 0) return 0.0;
    return static_cast<double>(busy_ticks_) / static_cast<double>(tick_count_);
}

uint64_t OnlineMetrics::completedJobs() const noexcept {
    return total_completed_;
}

uint64_t OnlineMetrics::arrivedJobs() const noexcept {
    return total_arrived_;
}

double OnlineMetrics::totalDecisionTimeNs() const noexcept {
    return total_decision_ns_;
}

double OnlineMetrics::meanDecisionTimeNs() const noexcept {
    if (tick_count_ == 0) return 0.0;
    return total_decision_ns_ / static_cast<double>(tick_count_);
}

uint64_t OnlineMetrics::currentTick() const noexcept {
    return tick_count_;
}

OnlineSnapshot OnlineMetrics::snapshot() const noexcept {
    return OnlineSnapshot{
        current_v_,
        drift_,
        rollingThroughput(),
        utilization(),
        total_completed_,
        tick_count_,
        hybridEmbiFraction()
    };
}

double OnlineMetrics::hybridEmbiFraction() const noexcept {
    uint64_t total_hybrid = hybrid_embi_ticks_ + hybrid_mw_ticks_;
    if (total_hybrid == 0) return 0.0;
    return static_cast<double>(hybrid_embi_ticks_) / static_cast<double>(total_hybrid);
}

const std::vector<int64_t>& OnlineMetrics::queueLengths() const noexcept {
    return queue_lengths_;
}

// ─── Reset ───────────────────────────────────────────────────────────────────

void OnlineMetrics::reset() {
    prev_v_            = 0.0;
    current_v_         = 0.0;
    drift_             = 0.0;
    window_head_       = 0;
    window_sum_        = 0;
    total_completed_   = 0;
    total_arrived_     = 0;
    busy_ticks_        = 0;
    tick_count_        = 0;
    total_decision_ns_ = 0.0;
    hybrid_embi_ticks_ = 0;
    hybrid_mw_ticks_   = 0;

    std::fill(completed_in_tick_.begin(), completed_in_tick_.end(), 0ULL);
    std::fill(queue_lengths_.begin(), queue_lengths_.end(), 0);
}

} // namespace embi
