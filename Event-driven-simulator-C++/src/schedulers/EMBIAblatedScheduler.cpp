#include "schedulers/EMBIAblatedScheduler.hpp"

#include "utils/Timer.hpp"
#include <algorithm>
#include <vector>

namespace embi {

EMBIAblatedScheduler::EMBIAblatedScheduler(const Config& config)
    : M_(config.M) {}

Decision EMBIAblatedScheduler::choose(const SchedulerContext& ctx) {
    const auto& procs = ctx.processes;
    const std::size_t N = procs.size();

    if (N == 0) return Decision::idle();

    Timer timer;

    std::vector<double> scores(N);
    for (std::size_t i = 0; i < N; ++i) {
        const Process& p = procs[i];
        // Omit the lambda predictive term
        double raw = p.mu_hat * (2.0 * static_cast<double>(p.queue_length) - M_);
        scores[i] = std::max(0.0, raw);
    }

    Decision d;
    argmaxWithQueues(scores, procs, d);

    if (!d.valid) return Decision::idle();

    computeDecisionDiagnostics(scores, d);
    d.decision_time_ns = timer.elapsed_ns();
    return d;
}

std::string_view EMBIAblatedScheduler::name() const noexcept {
    return "embi_ablated";
}

} // namespace embi
