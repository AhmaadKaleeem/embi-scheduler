#include "schedulers/GSQScheduler.hpp"

#include "utils/Timer.hpp"
#include <vector>

namespace embi {

Decision GSQScheduler::choose(const SchedulerContext& ctx) {
    const auto& procs = ctx.processes;
    const std::size_t N = procs.size();

    if (N == 0) return Decision::idle();

    Timer timer;

    std::vector<double> scores(N);
    for (std::size_t i = 0; i < N; ++i) {
        // GSQ prefers the process with the shortest queue length.
        // We use negative queue length so argmax picks the minimum.
        scores[i] = -static_cast<double>(procs[i].queue_length);
    }

    Decision d;
    argmaxWithQueues(scores, procs, d);

    if (!d.valid) return Decision::idle();

    computeDecisionDiagnostics(scores, d);
    d.decision_time_ns = timer.elapsed_ns();
    return d;
}

std::string_view GSQScheduler::name() const noexcept {
    return "gsq";
}

} // namespace embi
