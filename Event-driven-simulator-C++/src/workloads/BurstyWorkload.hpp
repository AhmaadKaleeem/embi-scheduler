/**
 * @file BurstyWorkload.hpp
 * @brief Markov ON/OFF bursty arrival process.
 *
 * Models a two-state Markov chain where the system alternates between:
 *   ON  state: high arrival rate (burst period)
 *   OFF state: low arrival rate (quiet period)
 *
 * State transitions per tick:
 *   P(ON  → OFF) = p_on_off
 *   P(OFF → ON)  = p_off_on
 *
 * Stationary ON probability: π_on = p_off_on / (p_on_off + p_off_on)
 *
 * Effective mean arrival rate:
 *   λ_eff = π_on · λ_on + (1 − π_on) · λ_off
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#pragma once

#include "workloads/BaseWorkload.hpp"
#include "utils/Random.hpp"

namespace embi {

/**
 * @class BurstyWorkload
 * @brief Markov ON/OFF two-state bursty arrival generator.
 *
 * @par Example
 * @code
 * embi::BurstyWorkload w(42,
 *     0.8,   // λ_on  (arrivals/tick in ON state)
 *     0.05,  // λ_off (arrivals/tick in OFF state)
 *     0.1,   // P(ON → OFF) per tick
 *     0.3);  // P(OFF → ON) per tick
 * @endcode
 */
class BurstyWorkload final : public BaseWorkload {
public:
    /**
     * @brief Constructs a BurstyWorkload.
     * @param seed       PRNG seed.
     * @param on_rate    Arrival rate in ON state (jobs/tick), must be ∈ (0, 1].
     * @param off_rate   Arrival rate in OFF state (jobs/tick), must be ∈ [0, 1].
     * @param p_on_off   Probability of transitioning ON→OFF per tick, ∈ (0, 1).
     * @param p_off_on   Probability of transitioning OFF→ON per tick, ∈ (0, 1).
     * @throws std::invalid_argument on invalid parameters.
     */
    BurstyWorkload(uint64_t seed,
                   double   on_rate,
                   double   off_rate,
                   double   p_on_off,
                   double   p_off_on);

    double           next()              override;
    void             seed(uint64_t s)    override;
    std::string_view name() const noexcept override;
    double           mean() const noexcept override;
    double           variance() const noexcept override;

    /// Returns true if the workload is currently in the ON (burst) state.
    [[nodiscard]] bool isOn() const noexcept;

    /// Returns the stationary probability of being in the ON state.
    [[nodiscard]] double stationaryOnProbability() const noexcept;

private:
    Random rng_;
    double on_rate_;
    double off_rate_;
    double p_on_off_;
    double p_off_on_;
    bool   state_on_{true};  // initial state: ON
};

} // namespace embi
