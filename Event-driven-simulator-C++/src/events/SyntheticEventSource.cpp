/**
 * @file SyntheticEventSource.cpp
 * @brief Implementation of the SyntheticEventSource.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#include "events/SyntheticEventSource.hpp"
#include "core/Event.hpp"

namespace embi {

SyntheticEventSource::SyntheticEventSource(const Config& config, std::unique_ptr<BaseWorkload> workload)
    : config_(config)
    , workload_(std::move(workload))
    , next_arrival_tick_(config.num_processes, 0.0)
{
    // Schedule first arrival for each process by sampling the workload
    for (std::size_t i = 0; i < config_.num_processes; ++i) {
        next_arrival_tick_[i] = workload_->next();
    }

    if (config_.workload_name == "lock_contention") {
        lock_workload_ = dynamic_cast<LockContentionWorkload*>(workload_.get());
        if (lock_workload_) {
            next_lock_req_tick_.resize(config_.num_processes);
            for (std::size_t i = 0; i < config_.num_processes; ++i) {
                next_lock_req_tick_[i] = lock_workload_->next();
            }
        }
    }
}

void SyntheticEventSource::emitEvents(double tick_d, EventQueue& queue) {
    // ── Generate arrival events ─────────────────────────────────────────
    for (std::size_t i = 0; i < config_.num_processes; ++i) {
        double rate = config_.arrival_rate;
        if (!config_.arrival_rate_asymmetric.empty()) {
            rate = config_.arrival_rate_asymmetric[i % config_.arrival_rate_asymmetric.size()];
        }
        double scale = (rate > 0.0) ? (config_.arrival_rate / rate) : 1.0;

        while (next_arrival_tick_[i] <= tick_d) {
            queue.push(makeArrivalEvent(tick_d, i));
            next_arrival_tick_[i] += workload_->next() * scale;
        }
    }

    // ── Generate LockAcquireEvents (lock_contention workload only) ──────
    if (lock_workload_) {
        for (std::size_t i = 0; i < config_.num_processes; ++i) {
            while (next_lock_req_tick_[i] <= tick_d) {
                std::size_t lock_id = lock_workload_->randomLockId();
                queue.push(makeLockAcquireEvent(tick_d, i, lock_id));
                next_lock_req_tick_[i] += lock_workload_->next();
            }
        }
    }
}

} // namespace embi
