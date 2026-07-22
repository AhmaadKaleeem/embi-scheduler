#include "schedulers/EMBIAblatedScheduler.hpp"

#include "utils/Timer.hpp"
#include <algorithm>
#include <vector>

namespace embi {

EMBIAblatedScheduler::EMBIAblatedScheduler(const Config& config, bool no_prediction, bool no_penalty)
    : M_(config.M), no_pred_(no_prediction), no_pen_(no_penalty) {}

Decision EMBIAblatedScheduler::choose(const SchedulerContext& ctx) {
    const auto& procs = ctx.processes;
    const std::size_t N = procs.size();

    if (N == 0) return Decision::idle();

    Timer timer;

    std::vector<double> scores(N);
    std::vector<double> raw_scores(N);
    for (std::size_t i = 0; i < N; ++i) {
        const Process& p = procs[i];
        
        double queue_term = 2.0 * static_cast<double>(p.queue_length + p.sync_debt);
        double pred_term = no_pred_ ? 0.0 : 2.0 * p.lambda_hat;
        double pen_term = no_pen_ ? 0.0 : M_;
        
        double raw = p.mu_hat * (queue_term + pred_term - pen_term);
        raw_scores[i] = raw;
        scores[i] = std::max(0.0, raw);
    }

    Decision d;
    d.raw_scores = raw_scores;
    d.final_scores = scores;
    argmaxWithQueues(scores, procs, d);

    if (!d.valid) return Decision::idle();
    
    // ── Compute hypothetical MaxWeight choice for diagnostics ────────────────
    std::size_t mw_pid = std::size_t(-1);
    double mw_max = -1.0;
    for (std::size_t i = 0; i < N; ++i) {
        const Process& p = procs[i];
        if (p.queue_length > 0) {
            double mw_score = p.mu_hat * static_cast<double>(p.queue_length + p.sync_debt);
            if (mw_score > mw_max) {
                mw_max = mw_score;
                mw_pid = i;
            } else if (mw_score == mw_max && i < mw_pid) {
                mw_pid = i;
            }
        }
    }
    d.differed_from_mw = (d.chosen_pid != mw_pid);
    
    const Process& chosen_p = procs[d.chosen_pid];
    d.queue_term = 2.0 * chosen_p.mu_hat * static_cast<double>(chosen_p.queue_length + chosen_p.sync_debt);
    d.prediction_term = no_pred_ ? 0.0 : 2.0 * chosen_p.lambda_hat * chosen_p.mu_hat;
    d.penalty_term = no_pen_ ? 0.0 : chosen_p.mu_hat * M_;
    d.raw_score = d.raw_scores[d.chosen_pid];
    d.clipped_score = d.final_scores[d.chosen_pid];

    computeDecisionDiagnostics(scores, d);
    d.decision_time_ns = timer.elapsed_ns();
    return d;
}

std::string_view EMBIAblatedScheduler::name() const noexcept {
    if (no_pred_ && !no_pen_) return "embi_no_prediction";
    if (!no_pred_ && no_pen_) return "embi_no_penalty";
    return "embi_ablated";
}

} // namespace embi
