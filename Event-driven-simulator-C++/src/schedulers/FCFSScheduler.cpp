/**
 * @file FCFSScheduler.cpp
 * @brief First-Come-First-Served scheduler implementation.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#include "schedulers/FCFSScheduler.hpp"

#include "utils/Timer.hpp"

#include <vector>

namespace embi {

FCFSScheduler::FCFSScheduler(const Config& /*config*/) {}

Decision FCFSScheduler::choose(const SchedulerContext& ctx) {
    const auto& procs = ctx.processes;
    const std::size_t N = procs.size();

    if (N == 0) return Decision::idle();

    Timer timer;
    const bool lock_mode = (ctx.config.workload_name == "lock_contention");

    std::size_t best_idx  = N;
    double      best_time = 0.0;

    for (std::size_t i = 0; i < N; ++i) {
        const Process& p = procs[i];
        if (p.queue_length <= 0) continue;
        if (lock_mode && !p.lock_state.holds_lock) continue;

        const double hol = p.holArrivalTime();
        if (hol < 0.0) continue;

        if (best_idx == N || hol < best_time ||
            (hol == best_time && i < best_idx)) {
            best_time = hol;
            best_idx = i;
        }
    }

    if (best_idx == N) return Decision::idle();

    // Finite wait-age scores keep diagnostics well-defined while preserving
    // the FCFS ordering used above.
    std::vector<double> scores(N, 0.0);
    for (std::size_t i = 0; i < N; ++i) {
        const Process& p = procs[i];
        if (p.queue_length <= 0) continue;
        if (lock_mode && !p.lock_state.holds_lock) continue;

        const double hol = p.holArrivalTime();
        if (hol >= 0.0) {
            scores[i] = ctx.current_tick - hol;
        }
    }

    Decision d;
    d.chosen_pid = best_idx;
    d.valid = true;

    computeDecisionDiagnostics(scores, d);
    d.decision_time_ns = timer.elapsed_ns();
    return d;
}

std::string_view FCFSScheduler::name() const noexcept {
    return "fcfs";
}

} // namespace embi
