/**
 * @file MaxWeightScheduler.cpp
 * @brief MaxWeight scheduler: score_i = μ̂_i · Q_i.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#include "schedulers/MaxWeightScheduler.hpp"

#include "utils/Timer.hpp"

#include <vector>

namespace embi {

MaxWeightScheduler::MaxWeightScheduler(const Config& config)
    : alpha_(config.alpha), beta_(config.beta) {}

Decision MaxWeightScheduler::choose(const SchedulerContext& ctx) {
    const auto& procs = ctx.processes;
    const std::size_t N = procs.size();

    if (N == 0) return Decision::idle();

    Timer timer;

    std::vector<double> scores(N);
    for (std::size_t i = 0; i < N; ++i) {
        // MaxWeight: service_rate × queue_length
        scores[i] = procs[i].mu_hat * static_cast<double>(procs[i].queue_length);
    }

    Decision d;
    argmaxWithQueues(scores, procs, d);

    if (!d.valid) return Decision::idle();

    computeDecisionDiagnostics(scores, d);
    d.decision_time_ns = timer.elapsed_ns();
    return d;
}

std::string_view MaxWeightScheduler::name() const noexcept {
    return "maxweight";
}

} // namespace embi
