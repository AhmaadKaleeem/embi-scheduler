/**
 * @file CmuScheduler.cpp
 * @brief cμ scheduler: score_i = μ̂_i.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#include "schedulers/CmuScheduler.hpp"

#include "utils/Timer.hpp"

#include <vector>

namespace embi {

CmuScheduler::CmuScheduler(const Config& /*config*/) {}

Decision CmuScheduler::choose(const SchedulerContext& ctx) {
    const auto& procs = ctx.processes;
    const std::size_t N = procs.size();

    if (N == 0) return Decision::idle();

    Timer timer;

    // cμ rule: score = estimated service rate (unit cost for all processes)
    std::vector<double> scores(N);
    for (std::size_t i = 0; i < N; ++i) {
        scores[i] = procs[i].mu_hat;
    }

    Decision d;
    argmaxWithQueues(scores, procs, d);

    if (!d.valid) return Decision::idle();

    computeDecisionDiagnostics(scores, d);
    d.decision_time_ns = timer.elapsed_ns();
    return d;
}

std::string_view CmuScheduler::name() const noexcept {
    return "cmu";
}

} // namespace embi
