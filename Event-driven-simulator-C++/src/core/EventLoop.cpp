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

    // Lock-contention mode: initialise LockManager and per-process request timers
    if (config.workload_name == "lock_contention") {
        lock_workload_ = dynamic_cast<LockContentionWorkload*>(workload_);
        if (lock_workload_) {
            lock_mgr_ = std::make_unique<LockManager>(config.num_locks);
            next_lock_req_tick_.resize(config.num_processes);
            for (std::size_t i = 0; i < config.num_processes; ++i) {
                next_lock_req_tick_[i] = lock_workload_->next();
            }
        }
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

        // ── Phase 1b: Generate LockAcquireEvents (lock_contention workload only) ──
        if (lock_mgr_) {
            for (std::size_t i = 0; i < processes_.size(); ++i) {
                while (next_lock_req_tick_[i] <= tick_d) {
                    std::size_t lock_id = lock_workload_->randomLockId();
                    event_queue_.push(makeLockAcquireEvent(tick_d, i, lock_id));
                    next_lock_req_tick_[i] += lock_workload_->next();
                }
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

                case EventType::LockAcquire:
                    if (lock_mgr_) handleLockAcquireEvent(e, tick);
                    break;

                case EventType::LockRelease:
                    if (lock_mgr_) handleLockReleaseEvent(e, tick);
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

    // ── Lock-contention: CPU-time accounting for the chosen process ───────────
    if (lock_mgr_ && last_decision_.valid) {
        const double tick_d = static_cast<double>(tick);
        std::size_t  pid    = last_decision_.chosen_pid;
        Process&     proc   = processes_[pid];

        if (proc.lock_state.holds_lock) {
            // Accumulate one CPU tick toward the critical section.
            proc.lock_state.elapsed_cpu += 1.0;

            // Release up to mu_hat waiters this tick.
            // min(Q, ceil(mu_hat)) — at least 1 if there is a waiter.
            const int64_t q = proc.queue_length;
            if (q > 0) {
                int64_t to_release = static_cast<int64_t>(proc.mu_hat);
                if (to_release < 1) to_release = 1;
                if (to_release > q) to_release = q;

                for (int64_t k = 0; k < to_release; ++k) {
                    double wt = proc.service(tick_d);
                    offline_metrics_->recordWaitingTime(wt);

                    // Promote next waiter in the LockManager's queue
                    // (this does NOT change ownership — proc still holds the lock;
                    //  unblocking means the waiter's Q entry is consumed).
                    // We simply call service() to dequeue from proc's job queue.
                }
            }

            // Check if critical section is complete.
            if (proc.lock_state.elapsed_cpu >= proc.lock_state.required_cpu) {
                int lock_id = proc.lock_state.held_lock_id;
                proc.lock_state = Process::LockState{};  // clear holding state

                // Release the lock and promote next waiter.
                int next_owner = lock_mgr_->release(static_cast<std::size_t>(lock_id));
                if (next_owner >= 0) {
                    promoteLockWaiter(static_cast<std::size_t>(lock_id),
                                      next_owner, tick_d);
                }
            }
        }
    }
}

void EventLoop::handleServiceEvent(const Event& e, uint64_t tick,
                                    double& waiting_time_out) {
    std::size_t pid = e.pid;
    if (pid >= processes_.size()) return;

    // In lock-contention mode, CPU-time service (waiter release) is handled
    // inside handleScheduleEvent. The regular ServiceEvent still handles
    // ordinary job completions for non-lock processes.
    if (!lock_mgr_ && processes_[pid].queue_length > 0) {
        double wt = processes_[pid].service(static_cast<double>(tick));
        waiting_time_out = wt;
        offline_metrics_->recordWaitingTime(wt);
    } else if (!lock_mgr_) {
        // No-op: empty queue.
    } else {
        // Lock-contention mode: service was already applied in handleScheduleEvent.
        // Still record a waiting time sample if available.
        waiting_time_out = 0.0;
    }
}

// ─── Lock event handlers ──────────────────────────────────────────────────────

void EventLoop::handleLockAcquireEvent(const Event& e, uint64_t tick) {
    std::size_t pid     = e.pid;
    std::size_t lock_id = static_cast<std::size_t>(e.payload);
    if (pid >= processes_.size() || lock_id >= lock_mgr_->numLocks()) return;

    const double tick_d = static_cast<double>(tick);

    bool acquired = lock_mgr_->acquire(lock_id, pid);

    if (acquired) {
        // Grant: set up the LockState with a fresh hold-duration sample.
        double hold = lock_workload_->holdDuration();
        processes_[pid].lock_state = Process::LockState{true,
                                                         static_cast<int>(lock_id),
                                                         hold,
                                                         0.0};
    } else {
        // Blocked: the current owner's Q_i increments — this IS the fluid-model
        // arrival update. We call arrival() on the owner.
        int owner = lock_mgr_->owner(lock_id);
        if (owner >= 0 && static_cast<std::size_t>(owner) < processes_.size()) {
            processes_[static_cast<std::size_t>(owner)].arrival(tick_d);
        }
    }
}

void EventLoop::handleLockReleaseEvent(const Event& e, uint64_t tick) {
    // This event is reserved for future use (e.g., preemptive release).
    // In the current CPU-time model, release is handled in handleScheduleEvent.
    (void)e; (void)tick;
}

void EventLoop::promoteLockWaiter(std::size_t lock_id, int new_owner_pid,
                                   double tick_d) {
    // The waiter is now the owner: give it a fresh critical-section hold duration.
    double hold = lock_workload_->holdDuration();
    std::size_t new_pid = static_cast<std::size_t>(new_owner_pid);
    if (new_pid < processes_.size()) {
        processes_[new_pid].lock_state = Process::LockState{true,
                                                              static_cast<int>(lock_id),
                                                              hold,
                                                              0.0};
    }
    (void)tick_d;
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
