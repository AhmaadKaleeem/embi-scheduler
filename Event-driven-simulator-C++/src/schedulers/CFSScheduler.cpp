/**
 * @file CFSScheduler.cpp
 * @brief Linux Completely Fair Scheduler (CFS) baseline implementation.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#include "schedulers/CFSScheduler.hpp"
#include "utils/Timer.hpp"

#include <algorithm>
#include <limits>
#include <vector>

namespace embi {

CFSScheduler::CFSScheduler(const Config& config) 
    : vruntimes_(config.num_processes, 0.0) {}

Decision CFSScheduler::choose(const SchedulerContext& ctx) {
    const auto& procs = ctx.processes;
    const std::size_t N = procs.size();

    if (N == 0) return Decision::idle();

    Timer timer;
    const bool lock_mode = (ctx.config.workload_name == "lock_contention");

    // Update vruntime for the previously chosen process
    if (ctx.previous_decision.has_value()) {
        std::size_t prev_pid = ctx.previous_decision.value();
        if (prev_pid < vruntimes_.size()) {
            vruntimes_[prev_pid] += 1.0;
        }
    }

    // Update min_vruntime_ to prevent newly awakened processes from monopolizing
    double current_min_vruntime = std::numeric_limits<double>::max();
    for (std::size_t i = 0; i < N; ++i) {
        if (procs[i].queue_length > 0) {
            if (lock_mode && !procs[i].lock_state.holds_lock) continue;
            if (vruntimes_[i] < current_min_vruntime) {
                current_min_vruntime = vruntimes_[i];
            }
        }
    }
    
    // If no active processes, go idle
    if (current_min_vruntime == std::numeric_limits<double>::max()) {
        return Decision::idle();
    }
    
    min_vruntime_ = current_min_vruntime;

    std::vector<double> scores(N, 0.0);
    bool any_valid = false;

    // A small threshold to prevent jobs that sleep for a long time from having an infinitely low vruntime.
    // In Linux CFS this is usually sysctl_sched_latency. We'll use a fixed conservative value.
    const double vruntime_bonus_cap = 10.0;

    for (std::size_t i = 0; i < N; ++i) {
        const Process& p = procs[i];
        if (p.queue_length <= 0) continue;
        if (lock_mode && !p.lock_state.holds_lock) continue;

        // If a process's vruntime is far behind the minimum of currently active processes
        if (vruntimes_[i] < min_vruntime_ - vruntime_bonus_cap) {
            vruntimes_[i] = min_vruntime_ - vruntime_bonus_cap;
        }

        // We want to pick the SMALLEST vruntime.
        // argmaxWithQueues picks the LARGEST score, so we use negative vruntime.
        scores[i] = -vruntimes_[i];
        any_valid = true;
    }

    if (!any_valid) return Decision::idle();

    Decision d;
    (void)argmaxWithQueues(scores, procs, d);
    
    if (d.valid) {
        computeDecisionDiagnostics(scores, d);
    }
    
    d.decision_time_ns = timer.elapsed_ns();
    return d;
}

std::string_view CFSScheduler::name() const noexcept {
    return "cfs";
}

} // namespace embi
