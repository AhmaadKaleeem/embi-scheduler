/**
 * @file BaseScheduler.cpp
 * @brief Shared scheduling utilities: diagnostics, softmax entropy, argmax.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#include "schedulers/BaseScheduler.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <stdexcept>

namespace embi {

// ─── computeDecisionDiagnostics ──────────────────────────────────────────────

void BaseScheduler::computeDecisionDiagnostics(const std::vector<double>& scores,
                                                Decision&                   d) {
    const std::size_t N = scores.size();
    if (N == 0) return;

    // ── Max and chosen score ─────────────────────────────────────────────────
    d.max_score    = *std::max_element(scores.begin(), scores.end());
    d.chosen_score = scores[d.chosen_pid];

    // ── Mean and variance (Welford's online algorithm) ───────────────────────
    double mean = 0.0;
    double M2   = 0.0;
    for (std::size_t i = 0; i < N; ++i) {
        double delta = scores[i] - mean;
        mean += delta / static_cast<double>(i + 1);
        M2   += delta * (scores[i] - mean);
    }
    d.mean_score     = mean;
    d.score_variance = (N > 1) ? M2 / static_cast<double>(N - 1) : 0.0;

    // ── Runner-up ────────────────────────────────────────────────────────────
    d.second_best_pid   = d.chosen_pid;
    d.second_best_score = -std::numeric_limits<double>::infinity();

    for (std::size_t i = 0; i < N; ++i) {
        if (i == d.chosen_pid) continue;
        if (scores[i] > d.second_best_score) {
            d.second_best_score = scores[i];
            d.second_best_pid   = i;
        }
    }
    d.score_delta = d.chosen_score - d.second_best_score;

    // ── Decision entropy (log-sum-exp stable softmax Shannon entropy) ─────────
    // H = −Σ p_i · ln(p_i),  p_i = exp(score_i) / Σ exp(score_j)
    // Numerically stable: subtract max before computing exp.
    const double max_s  = d.max_score;
    double log_sum_exp  = 0.0;
    for (double s : scores) {
        log_sum_exp += std::exp(s - max_s);
    }
    log_sum_exp = std::log(log_sum_exp) + max_s;

    double entropy = 0.0;
    for (double s : scores) {
        double log_p_i = s - log_sum_exp;  // ln(p_i)
        double p_i     = std::exp(log_p_i);
        if (p_i > 0.0) {
            entropy -= p_i * log_p_i;
        }
    }
    d.decision_entropy = entropy;
}

// ─── argmaxWithQueues ────────────────────────────────────────────────────────

std::size_t BaseScheduler::argmaxWithQueues(const std::vector<double>&  scores,
                                             const std::vector<Process>& processes,
                                             Decision&                    d) {
    const std::size_t N = scores.size();

    std::size_t best_idx   = 0;
    double      best_score = -std::numeric_limits<double>::infinity();
    bool        any_valid  = false;

    for (std::size_t i = 0; i < N; ++i) {
        if (processes[i].queue_length <= 0) continue;  // skip empty queues
        if (scores[i] > best_score) {
            best_score = scores[i];
            best_idx   = i;
            any_valid  = true;
        }
    }

    d.valid      = any_valid;
    d.chosen_pid = best_idx;
    return best_idx;
}

} // namespace embi
