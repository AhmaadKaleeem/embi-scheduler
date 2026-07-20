/**
 * @file RoundRobinScheduler.cpp
 * @brief Round-robin scheduler implementation.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#include "schedulers/RoundRobinScheduler.hpp"

#include "utils/Timer.hpp"

#include <vector>

namespace embi {

RoundRobinScheduler::RoundRobinScheduler(const Config& /*config*/)
    : current_index_(0) {}

Decision RoundRobinScheduler::choose(const SchedulerContext& ctx) {
    const auto& procs = ctx.processes;
    const std::size_t N = procs.size();

    if (N == 0) return Decision::idle();

    Timer timer;

    // Find next non-empty process starting from current_index_
    // Advance up to N steps; if no non-empty process found, go idle.
    std::size_t start = current_index_;
    std::size_t chosen = N;  // sentinel: invalid

    for (std::size_t attempt = 0; attempt < N; ++attempt) {
        std::size_t idx = (start + attempt) % N;
        if (procs[idx].queue_length > 0) {
            chosen = idx;
            current_index_ = (idx + 1) % N;  // advance for next call
            break;
        }
    }

    if (chosen == N) {
        // All queues empty
        current_index_ = (current_index_ + 1) % N;
        return Decision::idle();
    }

    // Build uniform scores for diagnostic (RR doesn't rank by score)
    std::vector<double> scores(N, 0.0);
    scores[chosen] = 1.0;  // chosen gets score 1, others 0

    Decision d;
    d.chosen_pid = chosen;
    d.valid      = true;

    computeDecisionDiagnostics(scores, d);
    d.decision_time_ns = timer.elapsed_ns();
    return d;
}

std::string_view RoundRobinScheduler::name() const noexcept {
    return "rr";
}

} // namespace embi
