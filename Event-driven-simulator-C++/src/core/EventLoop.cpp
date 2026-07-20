/**
 * @file EventLoop.cpp
 * @brief Implementation of the discrete-event simulation engine.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#include "core/EventLoop.hpp"

#include "utils/Timer.hpp"

#include <algorithm>
#include <cassert>
#include <stdexcept>

namespace embi {


EventLoop::EventLoop(const Config&       config,
                     BaseWorkload*       workload,
                     BaseScheduler*      scheduler,
                     OnlineMetrics*      online_metrics,
                     OfflineMetrics*     offline_metrics,
                     StatisticsDatabase& stats_db)
    : config_(config)
    , workload_(workload)
    , scheduler_(scheduler)
    , online_metrics_(online_metrics)
    , offline_metrics_(offline_metrics)
    , stats_db_(stats_db)
    , event_queue_(static_cast<std::size_t>(config.num_processes) * 4 + 16)
    , next_arrival_tick_(config.num_processes, 0.0)
{
    // Pre-allocate processes
    processes_.reserve(config.num_processes);
    for (std::size_t i = 0; i < config.num_processes; ++i) {
        double rate = config.arrival_rate;
        if (!config.arrival_rate_asymmetric.empty()) {
            rate = config.arrival_rate_asymmetric[i % config.arrival_rate_asymmetric.size()];
        }
        processes_.emplace_back(i,
                                rate,
                                config.service_rate,
                                config.alpha,
                                config.beta);
    }

    // Schedule first arrival for each process by sampling the workload
    for (std::size_t i = 0; i < config.num_processes; ++i) {
        next_arrival_tick_[i] = workload_->next();
    }
}

// ─── Main loop ───────────────────────────────────────────────────────────────

void EventLoop::run() {
    const uint64_t total_ticks = config_.ticks;
    const uint64_t log_freq    = config_.log_freq;

    for (uint64_t tick = 0; tick < total_ticks; ++tick) {
        const double tick_d = static_cast<double>(tick);

        // ── Phase 1: Generate arrival events ─────────────────────────────────
        // For each process, push an ArrivalEvent for every pending arrival.
        // This handles the case where multiple arrivals accumulate between ticks
        // (e.g., in heavy-tail workloads with very small inter-arrival times).
        for (std::size_t i = 0; i < processes_.size(); ++i) {
            double rate = config_.arrival_rate;
            if (!config_.arrival_rate_asymmetric.empty()) {
                rate = config_.arrival_rate_asymmetric[i % config_.arrival_rate_asymmetric.size()];
            }
            double scale = (rate > 0.0) ? (config_.arrival_rate / rate) : 1.0;

            while (next_arrival_tick_[i] <= tick_d) {
                event_queue_.push(makeArrivalEvent(tick_d, i));
                next_arrival_tick_[i] += workload_->next() * scale;
            }
        }

        // ── Phase 2: Push Schedule and Metrics events ─────────────────────────
        event_queue_.push(makeScheduleEvent(tick_d));
        event_queue_.push(makeMetricsEvent(tick_d));

        // ── Phase 3: Drain all events at this tick ────────────────────────────
        // Events are ordered: Arrival(0) < Schedule(1) < Service(2) < Metrics(3)
        double waiting_time_this_tick = 0.0;

        while (!event_queue_.empty() &&
               event_queue_.top().timestamp <= tick_d) {

            Event e = event_queue_.pop();

            switch (e.type) {
                case EventType::Arrival:
                    handleArrivalEvent(e, tick);
                    break;

                case EventType::Schedule:
                    handleScheduleEvent(e, tick);
                    // If the scheduler chose a process, push a ServiceEvent
                    if (last_decision_.valid) {
                        event_queue_.push(makeServiceEvent(tick_d, last_decision_.chosen_pid));
                    }
                    break;

                case EventType::Service:
                    handleServiceEvent(e, tick, waiting_time_this_tick);
                    break;

                case EventType::Metrics:
                    handleMetricsEvent(e, tick, waiting_time_this_tick);
                    break;
            }
        }

        // ── Phase 4: Emit log records at configured frequency ─────────────────
        if (tick % log_freq == 0) {
            emitLogRecords(tick, waiting_time_this_tick);
        }
    }

    // Flush logger at end of simulation
    stats_db_.flush();
}

// ─── Event handlers ──────────────────────────────────────────────────────────

void EventLoop::handleArrivalEvent(const Event& e, uint64_t tick) {
    std::size_t pid = e.pid;
    if (pid >= processes_.size()) return;

    processes_[pid].arrival(static_cast<double>(tick));
}

void EventLoop::handleScheduleEvent(const Event& /*e*/, uint64_t tick) {
    // Build context
    OnlineSnapshot snapshot = online_metrics_->snapshot();
    SchedulerContext ctx{
        processes_,
        static_cast<double>(tick),
        prev_decision_,
        snapshot,
        config_
    };

    // Call scheduler
    last_decision_ = scheduler_->choose(ctx);

    // Update previous decision
    if (last_decision_.valid) {
        prev_decision_ = last_decision_.chosen_pid;
    }
}

void EventLoop::handleServiceEvent(const Event& e, uint64_t tick,
                                    double& waiting_time_out) {
    std::size_t pid = e.pid;
    if (pid >= processes_.size()) return;

    if (processes_[pid].queue_length > 0) {
        double wt = processes_[pid].service(static_cast<double>(tick));
        waiting_time_out = wt;

        // Record waiting time in offline metrics
        offline_metrics_->recordWaitingTime(wt);
    }
}

void EventLoop::handleMetricsEvent(const Event& /*e*/, uint64_t tick,
                                    double waiting_time) {
    // Update online metrics
    online_metrics_->update(processes_, last_decision_, tick);

    // Periodically record queue snapshots and decisions for offline analysis
    // Sampling stride: every 10 ticks to keep memory bounded
    if (tick % 10 == 0) {
        offline_metrics_->recordQueueSnapshot(processes_, tick);
    }
    offline_metrics_->recordDecision(last_decision_, tick);
    offline_metrics_->recordDrift(online_metrics_->lyapunovDrift());

    // Update starvation counters for unchosen processes
    for (std::size_t i = 0; i < processes_.size(); ++i) {
        if (!last_decision_.valid || last_decision_.chosen_pid != i) {
            processes_[i].tickIdle();
        }
    }

    (void)waiting_time;
}

// ─── Log emission ─────────────────────────────────────────────────────────────

void EventLoop::emitLogRecords(uint64_t tick, double waiting_time) {
    OnlineSnapshot snap = online_metrics_->snapshot();

    for (std::size_t i = 0; i < processes_.size(); ++i) {
        const Process& p = processes_[i];

        LogRecord rec;
        rec.tick            = tick;
        rec.pid             = i;
        rec.queue_length    = p.queue_length;
        rec.arrival_rate    = p.true_arrival_rate;
        rec.service_rate    = p.true_service_rate;
        rec.lambda_hat      = p.lambda_hat;
        rec.mu_hat          = p.mu_hat;
        rec.scheduler_score = 0.0;  // computed per-scheduler in Decision; store separately
        rec.chosen          = (last_decision_.valid && last_decision_.chosen_pid == i);
        rec.waiting_time    = rec.chosen ? waiting_time : 0.0;
        rec.completion_time = p.last_service_time;
        rec.throughput      = snap.throughput;
        rec.lyapunov_v      = snap.lyapunov_v;
        rec.lyapunov_drift  = snap.lyapunov_drift;

        stats_db_.record(rec);
    }
}

// ─── Process access ───────────────────────────────────────────────────────────

const std::vector<Process>& EventLoop::processes() const noexcept {
    return processes_;
}

} // namespace embi
