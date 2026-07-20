/**
 * @file BurstyWorkload.cpp
 * @brief Implementation of BurstyWorkload (Markov ON/OFF model).
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#include "workloads/BurstyWorkload.hpp"

#include <stdexcept>

namespace embi {

BurstyWorkload::BurstyWorkload(uint64_t seed_val,
                               double   on_rate,
                               double   off_rate,
                               double   p_on_off,
                               double   p_off_on)
    : rng_(seed_val)
    , on_rate_(on_rate)
    , off_rate_(off_rate)
    , p_on_off_(p_on_off)
    , p_off_on_(p_off_on)
    , state_on_(true)
{
    if (on_rate  <= 0.0 || on_rate  > 1.0)
        throw std::invalid_argument("BurstyWorkload: on_rate must be in (0, 1]");
    if (off_rate < 0.0  || off_rate > 1.0)
        throw std::invalid_argument("BurstyWorkload: off_rate must be in [0, 1]");
    if (p_on_off <= 0.0 || p_on_off >= 1.0)
        throw std::invalid_argument("BurstyWorkload: p_on_off must be in (0, 1)");
    if (p_off_on <= 0.0 || p_off_on >= 1.0)
        throw std::invalid_argument("BurstyWorkload: p_off_on must be in (0, 1)");
}

double BurstyWorkload::next() {
    // Step 1: Transition the Markov chain state
    if (state_on_) {
        if (rng_.bernoulli(p_on_off_)) {
            state_on_ = false;
        }
    } else {
        if (rng_.bernoulli(p_off_on_)) {
            state_on_ = true;
        }
    }

    // Step 2: Generate inter-arrival time from current state's rate
    // Rate 0 in OFF state means no arrivals; return a very large inter-arrival.
    double rate = state_on_ ? on_rate_ : off_rate_;
    if (rate <= 0.0) {
        // No arrivals in OFF state: return a very large inter-arrival time
        return 1.0e12;
    }
    return rng_.exponential(rate);
}

void BurstyWorkload::seed(uint64_t s) {
    rng_.reseed(s);
    state_on_ = true;  // Reset to ON state on re-seed
}

std::string_view BurstyWorkload::name() const noexcept {
    return "bursty";
}

double BurstyWorkload::stationaryOnProbability() const noexcept {
    return p_off_on_ / (p_on_off_ + p_off_on_);
}

double BurstyWorkload::mean() const noexcept {
    // Mean inter-arrival = 1 / effective_rate
    double pi_on     = stationaryOnProbability();
    double eff_rate  = pi_on * on_rate_ + (1.0 - pi_on) * off_rate_;
    if (eff_rate <= 0.0) return 1.0e12;
    return 1.0 / eff_rate;
}

double BurstyWorkload::variance() const noexcept {
    // Variance of a mixture of two exponentials:
    // Var[X] = E[X²] − (E[X])²
    // E[X²] = π_on · (2/λ_on²) + (1−π_on) · (2/λ_off²)
    double pi_on    = stationaryOnProbability();
    double pi_off   = 1.0 - pi_on;
    double ex2_on   = (on_rate_  > 0.0) ? 2.0 / (on_rate_  * on_rate_)  : 0.0;
    double ex2_off  = (off_rate_ > 0.0) ? 2.0 / (off_rate_ * off_rate_) : 0.0;
    double ex2      = pi_on * ex2_on + pi_off * ex2_off;
    double ex       = mean();
    return ex2 - ex * ex;
}

bool BurstyWorkload::isOn() const noexcept {
    return state_on_;
}

} // namespace embi
