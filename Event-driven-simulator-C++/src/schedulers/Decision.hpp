/**
 * @file Decision.hpp
 * @brief Scheduler decision record with full diagnostic fields.
 *
 * Every call to BaseScheduler::choose() returns a Decision struct capturing
 * not just which process was chosen, but the complete score landscape:
 * the chosen and runner-up scores, the score distribution entropy, and the
 * wall-clock time spent making the decision.
 *
 * This makes the simulator a first-class diagnostic tool — reviewers can
 * inspect *why* the scheduler made every decision, not just *what* it chose.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

namespace embi {

/**
 * @struct Decision
 * @brief Full diagnostic output of a single scheduler invocation.
 *
 * @par Entropy
 * decision_entropy is the Shannon entropy (in nats) of the softmax distribution
 * over all process scores:
 *
 *   p_i = exp(score_i) / Σ exp(score_j)
 *   H   = −Σ p_i · ln(p_i)
 *
 * High entropy → scores are similar (low confidence).
 * H = 0         → one process dominates all others (maximum confidence).
 *
 * Uses log-sum-exp for numerical stability.
 *
 * @par Validity
 * If no process has queue_length > 0, or all scores are ≤ 0 (EMBI clipped),
 * the field valid is set to false. The EventLoop skips the service event in
 * this case (CPU goes idle).
 */
struct Decision {
    // ─── Primary choice ───────────────────────────────────────────────────────
    std::size_t chosen_pid{0};       ///< Index of the chosen process.
    double      chosen_score{0.0};   ///< Score of the chosen process.

    // ─── Runner-up ────────────────────────────────────────────────────────────
    std::size_t second_best_pid{0};        ///< Index of the process with second-highest score.
    double      second_best_score{0.0};    ///< Score of the runner-up process.
    double      score_delta{0.0};          ///< chosen_score − second_best_score.

    // ─── Score distribution summary ───────────────────────────────────────────
    double max_score{0.0};        ///< Maximum score across all processes.
    double mean_score{0.0};       ///< Arithmetic mean of all scores.
    double score_variance{0.0};   ///< Variance of the score distribution.
    double decision_entropy{0.0}; ///< Shannon entropy of softmax(scores) (nats).
    std::vector<double> raw_scores;   ///< Raw computed scores for all processes
    std::vector<double> final_scores; ///< Final (e.g. clipped) scores for all processes

    // ─── Performance ──────────────────────────────────────────────────────────
    double decision_time_ns{0.0}; ///< Wall-clock time for choose() in nanoseconds.

    // ─── Validity ─────────────────────────────────────────────────────────────
    bool valid{false}; ///< False if CPU is idle this tick (no eligible process).

    // ─── Diagnostic flags ──────────────────────────────────────────────────────
    int mode_flag{0};  ///< 0: default, 1: MW fallback, 2: EMBI confidence
    
    // ─── Hybrid Metrics (New) ─────────────────────────────────────────────────
    double S_X{0.0};              ///< Total queue length sum S(X)
    double tau_X{0.0};            ///< Computed threshold tau(X)
    double g_hat_X{0.0};          ///< Computed raw gap g_hat(X)
    double raw_top_score{0.0};    ///< Raw score of the top process
    double raw_second_score{0.0}; ///< Raw score of the runner-up process
    double eta_c{0.0};            ///< Diagnostic clipped ratio eta_c
    int    fallback_reason{0};    ///< 0: none, 1: gap <= tau (fallback), 2: gap > tau (embi)

    // ─── Score Decomposition (New) ────────────────────────────────────────────
    double queue_term{0.0};       ///< The queue length component (2 * mu * Q)
    double prediction_term{0.0};  ///< The prediction component (2 * lambda * mu)
    double penalty_term{0.0};     ///< The penalty component (M * mu^2)
    double raw_score{0.0};        ///< Unclipped raw score sum
    double clipped_score{0.0};    ///< Final score after clipping (max(0, raw))

    // ─── Ranking diagnostics ──────────────────────────────────────────────────
    bool differed_from_mw{false}; ///< True if the chosen PID differed from MaxWeight's choice


    // ─── Convenience ─────────────────────────────────────────────────────────

    /// Returns a sentinel Decision representing a CPU-idle tick.
    [[nodiscard]] static Decision idle() noexcept {
        Decision d;
        d.valid = false;
        return d;
    }
};

} // namespace embi
