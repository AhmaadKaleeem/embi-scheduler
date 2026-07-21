/**
 * @file HybridEMBIScheduler.cpp
 * @brief Hybrid EMBI scheduler implementation.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#include "schedulers/HybridEMBIScheduler.hpp"

#include "utils/Timer.hpp"

#include <algorithm>
#include <vector>

namespace embi {

HybridEMBIScheduler::HybridEMBIScheduler(const Config& config)
    : M_(config.M), 
      epsilon_total_(config.epsilon_total), 
      tau_constant_bound_(config.tau_constant_bound) {}

Decision HybridEMBIScheduler::choose(const SchedulerContext& ctx) {
    const auto& procs = ctx.processes;
    const std::size_t N = procs.size();

    if (N == 0) return Decision::idle();

    Timer timer;

    // ── 1. Compute per-process EMBI scores and S(X) ──────────────────────────
    std::vector<double> raw_embi_scores(N);
    std::vector<double> clipped_embi_scores(N);
    
    double S_X = 0.0;
    
    for (std::size_t i = 0; i < N; ++i) {
        const Process& p = procs[i];
        
        // Sum total fluid queue length (S_X)
        S_X += static_cast<double>(p.queue_length + p.sync_debt);

        // Compute raw score
        double raw = p.mu_hat * (2.0 * static_cast<double>(p.queue_length + p.sync_debt)
                                 + 2.0 * p.lambda_hat
                                 - M_);
        raw_embi_scores[i] = raw;
        clipped_embi_scores[i] = std::max(0.0, raw);
    }

    // Identify the best process based on RAW scores for the decision rule
    Decision raw_d;
    argmaxWithQueues(raw_embi_scores, procs, raw_d);
    
    if (!raw_d.valid) return Decision::idle();
    computeDecisionDiagnostics(raw_embi_scores, raw_d);

    // Identify the best process based on CLIPPED scores for diagnostics & returning
    Decision clipped_d;
    argmaxWithQueues(clipped_embi_scores, procs, clipped_d);
    computeDecisionDiagnostics(clipped_embi_scores, clipped_d);

    // ── 2. Confidence-Gated Fallback Logic ───────────────────────────────────
    
    // Calculate the threshold tau(X)
    double tau_X = 2.0 * (epsilon_total_ * S_X + tau_constant_bound_);
    
    // The raw gap g_hat(X)
    double g_hat_X = raw_d.score_delta; // score_delta is already s1 - s2
    
    // The diagnostic clipped ratio eta_c
    double c_s1 = clipped_d.chosen_score;
    double c_s2 = c_s1 - clipped_d.score_delta;
    double eta_c = clipped_d.score_delta / (c_s1 + c_s2 + 1e-9);

    if (g_hat_X > tau_X) {
        // High confidence: Signal dominates noise. Use EMBI.
        clipped_d.mode_flag = 2; // 2 = EMBI confidence
        clipped_d.decision_time_ns = timer.elapsed_ns();
        
        // Log new diagnostics
        clipped_d.S_X = S_X;
        clipped_d.tau_X = tau_X;
        clipped_d.g_hat_X = g_hat_X;
        clipped_d.raw_top_score = raw_d.chosen_score;
        clipped_d.raw_second_score = raw_d.chosen_score - raw_d.score_delta;
        clipped_d.eta_c = eta_c;
        clipped_d.fallback_reason = 2; // gap > tau
        clipped_d.raw_scores = raw_embi_scores;
        clipped_d.final_scores = clipped_embi_scores;
        
        return clipped_d;
    }

    // Low confidence: Noise dominates signal. Fall back to queue-only.
    // The paper defines the variance-reduction fallback strictly as argmax Q_i.
    std::vector<double> fallback_scores(N);
    for (std::size_t i = 0; i < N; ++i) {
        const Process& p = procs[i];
        fallback_scores[i] = static_cast<double>(p.queue_length + p.sync_debt);
    }

    Decision fallback_d;
    argmaxWithQueues(fallback_scores, procs, fallback_d);
    computeDecisionDiagnostics(fallback_scores, fallback_d);
    
    fallback_d.mode_flag = 1; // 1 = fallback
    fallback_d.decision_time_ns = timer.elapsed_ns();
    
    // Log new diagnostics
    fallback_d.S_X = S_X;
    fallback_d.tau_X = tau_X;
    fallback_d.g_hat_X = g_hat_X;
    fallback_d.raw_top_score = raw_d.chosen_score;
    fallback_d.raw_second_score = raw_d.chosen_score - raw_d.score_delta;
    fallback_d.eta_c = eta_c;
    fallback_d.fallback_reason = 1; // gap <= tau
    fallback_d.raw_scores = raw_embi_scores;
    fallback_d.final_scores = fallback_scores;

    return fallback_d;
}

std::string_view HybridEMBIScheduler::name() const noexcept {
    return "hybrid_embi";
}

} // namespace embi
