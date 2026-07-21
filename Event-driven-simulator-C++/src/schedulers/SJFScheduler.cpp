/**
 * @file SJFScheduler.cpp
 * @brief Shortest Expected Job First (SJF) scheduler implementation.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#include "schedulers/SJFScheduler.hpp"
#include "utils/Timer.hpp"

#include <vector>

namespace embi {

SJFScheduler::SJFScheduler(const Config& /*config*/) {}

Decision SJFScheduler::choose(const SchedulerContext& ctx) {
    const auto& procs = ctx.processes;
    const std::size_t N = procs.size();

    if (N == 0) return Decision::idle();

    Timer timer;
    const bool lock_mode = (ctx.config.workload_name == "lock_contention");

    std::vector<double> scores(N, 0.0);
    bool any_valid = false;

    for (std::size_t i = 0; i < N; ++i) {
        const Process& p = procs[i];
        if (p.queue_length <= 0) continue;
        if (lock_mode && !p.lock_state.holds_lock) continue;

        // SJF prioritises jobs with highest expected service rate (mu_hat)
        // which corresponds to the shortest expected job duration (1 / mu_hat)
        scores[i] = p.mu_hat;
        any_valid = true;
    }

    if (!any_valid) return Decision::idle();

    Decision d;
    (void)argmaxWithQueues(scores, procs, d);
    
    // argmaxWithQueues checks queue lengths, but we already set scores to 0.0
    // for empty queues. Still, if d.valid is true, we compute diagnostics.
    if (d.valid) {
        computeDecisionDiagnostics(scores, d);
    }
    
    d.decision_time_ns = timer.elapsed_ns();
    return d;
}

std::string_view SJFScheduler::name() const noexcept {
    return "sjf";
}

} // namespace embi
