/**
 * @file EMBIScheduler.cpp
 * @brief EMBI scheduler implementation.
 *
 * score_i = clip(μ̂_i · (2·Q_i + 2·λ̂_i − M))
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#include "schedulers/EMBIScheduler.hpp"

#include "utils/Timer.hpp"

#include <algorithm>
#include <vector>

namespace embi {

EMBIScheduler::EMBIScheduler(const Config& config, bool clip)
    : M_(config.M), clip_(clip) {}

Decision EMBIScheduler::choose(const SchedulerContext& ctx) {
    const auto& procs = ctx.processes;
    const std::size_t N = procs.size();

    if (N == 0) return Decision::idle();

    Timer timer;

    // ── Compute per-process EMBI scores ──────────────────────────────────────
    std::vector<double> scores(N);
    std::vector<double> raw_scores(N);
    for (std::size_t i = 0; i < N; ++i) {
        const Process& p = procs[i];
        double raw = p.mu_hat * (2.0 * static_cast<double>(p.queue_length + p.sync_debt)
                                 + 2.0 * p.lambda_hat
                                 - M_);
        raw_scores[i] = raw;
        scores[i] = clip_ ? std::max(0.0, raw) : raw;
    }

    Decision d;
    d.raw_scores = raw_scores;
    d.final_scores = scores;
    argmaxWithQueues(scores, procs, d);

    if (!d.valid) return Decision::idle();

    computeDecisionDiagnostics(scores, d);
    d.decision_time_ns = timer.elapsed_ns();
    return d;
}

std::string_view EMBIScheduler::name() const noexcept {
    return clip_ ? "embi" : "embi_unclipped";
}

} // namespace embi
