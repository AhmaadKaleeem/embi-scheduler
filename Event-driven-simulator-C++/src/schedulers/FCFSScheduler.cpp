/**
 * @file FCFSScheduler.cpp
 * @brief First-Come-First-Served scheduler implementation.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#include "schedulers/FCFSScheduler.hpp"

#include "utils/Timer.hpp"

#include <limits>
#include <vector>

namespace embi {

FCFSScheduler::FCFSScheduler(const Config& /*config*/) {}

Decision FCFSScheduler::choose(const SchedulerContext& ctx) {
    const auto& procs = ctx.processes;
    const std::size_t N = procs.size();

    if (N == 0) return Decision::idle();

    Timer timer;

    // FCFS: choose the process with the earliest first_arrival_time.
    // Processes that have never had an arrival (first_arrival_time < 0)
    // are treated as having infinite wait → not eligible.
    // Tie-break by process ID (lower ID wins).
    std::size_t best_idx    = N;
    double      best_time   = std::numeric_limits<double>::infinity();

    for (std::size_t i = 0; i < N; ++i) {
        if (procs[i].queue_length <= 0) continue;
        if (procs[i].holArrivalTime() < 0.0) continue;

        // Earlier first arrival = higher priority
        if (procs[i].holArrivalTime() < best_time ||
            (procs[i].holArrivalTime() == best_time && i < best_idx)) {
            best_time = procs[i].holArrivalTime();
            best_idx  = i;
        }
    }

    if (best_idx == N) return Decision::idle();

    // Scores: assign each process its negative arrival time as a score
    // (higher score = earlier arrival, making argmax equivalent to FCFS).
    std::vector<double> scores(N, 0.0);
    for (std::size_t i = 0; i < N; ++i) {
        if (procs[i].queue_length > 0 && procs[i].holArrivalTime() >= 0.0) {
            scores[i] = -procs[i].holArrivalTime();  // negate: earlier → higher score
        } else {
            scores[i] = -std::numeric_limits<double>::infinity();
        }
    }

    Decision d;
    d.chosen_pid = best_idx;
    d.valid      = true;

    computeDecisionDiagnostics(scores, d);
    d.decision_time_ns = timer.elapsed_ns();
    return d;
}

std::string_view FCFSScheduler::name() const noexcept {
    return "fcfs";
}

} // namespace embi
