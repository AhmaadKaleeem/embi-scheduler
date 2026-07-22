/**
 * @file EMBIScheduler.cpp
 * @brief EMBI scheduler implementation using the estimator framework.
 *
 * @author  EMBI Simulator Project
 * @version 2.0.0
 */

#include "schedulers/EMBIScheduler.hpp"
#include "utils/Timer.hpp"
#include <algorithm>

namespace embi {

EMBIScheduler::EMBIScheduler(std::unique_ptr<EMBIEstimator> estimator)
    : estimator_(std::move(estimator)) {}

Decision EMBIScheduler::choose(const SchedulerContext& ctx) {
    const auto& procs = ctx.processes;
    const std::size_t N = procs.size();

    if (N == 0) return Decision::idle();

    Timer timer;

    // Estimate EMBI for all processes
    std::vector<double> scores = estimator_->estimate(procs, *ctx.graph_state);

    Decision d;
    argmaxWithQueues(scores, procs, d);

    if (!d.valid) return Decision::idle();

    computeDecisionDiagnostics(scores, d);
    d.decision_time_ns = timer.elapsed_ns();
    d.mode_flag = 4; // Framework mode

    return d;
}

std::string_view EMBIScheduler::name() const noexcept {
    return "embi_framework";
}

} // namespace embi
