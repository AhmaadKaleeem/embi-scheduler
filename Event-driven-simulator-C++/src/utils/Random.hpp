/**
 * @file Random.hpp
 * @brief Deterministic pseudo-random number generator and distribution sampler.
 *
 * Wraps std::mt19937_64 with a clean interface supporting all distributions
 * required by the workload generators. Every instance is independently seeded,
 * enabling deterministic replay and parallel experiment sweeps.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#pragma once

#include <cstdint>
#include <limits>
#include <random>
#include <stdexcept>

namespace embi {

/**
 * @class Random
 * @brief Seeded PRNG wrapper with statistical distribution samplers.
 *
 * All methods are thread-UNSAFE by design; each simulation thread should
 * own its own Random instance (no locking overhead in the hot path).
 *
 * @par Example
 * @code
 * embi::Random rng(42);
 * double rate = rng.exponential(0.5);   // Exp(rate=0.5) => mean = 2.0
 * bool   hit  = rng.bernoulli(0.3);     // true with prob 0.3
 * @endcode
 */
class Random {
public:
    // ─── Construction ────────────────────────────────────────────────────────

    /**
     * @brief Constructs a Random object with the given seed.
     * @param seed  64-bit seed; use the same seed for reproducibility.
     *
     * @complexity O(1) amortised (Mersenne Twister warmup is constant).
     */
    explicit Random(uint64_t seed = 0) noexcept;

    Random(const Random&)            = delete;
    Random& operator=(const Random&) = delete;
    Random(Random&&)                 = default;
    Random& operator=(Random&&)      = default;

    // ─── State management ────────────────────────────────────────────────────

    /**
     * @brief Re-seeds the underlying Mersenne Twister engine.
     * @param seed  New 64-bit seed.
     * @complexity O(1)
     */
    void reseed(uint64_t seed) noexcept;

    /**
     * @brief Returns the current seed (as originally set; not the engine state).
     * @return Seed value passed to the constructor or the last reseed() call.
     */
    [[nodiscard]] uint64_t currentSeed() const noexcept;

    // ─── Uniform distributions ───────────────────────────────────────────────

    /**
     * @brief Samples from a continuous uniform distribution U(lo, hi).
     * @param lo  Lower bound (inclusive).
     * @param hi  Upper bound (exclusive).
     * @return Sample in [lo, hi).
     * @throws std::invalid_argument if lo >= hi.
     * @complexity O(1)
     */
    double uniformReal(double lo, double hi);

    /**
     * @brief Samples from a discrete uniform distribution over [lo, hi].
     * @param lo  Lower bound (inclusive).
     * @param hi  Upper bound (inclusive).
     * @return Integer sample in [lo, hi].
     * @throws std::invalid_argument if lo > hi.
     * @complexity O(1)
     */
    int64_t uniformInt(int64_t lo, int64_t hi);

    // ─── Bernoulli ───────────────────────────────────────────────────────────

    /**
     * @brief Samples from a Bernoulli distribution.
     * @param p  Success probability in [0, 1].
     * @return  true with probability p, false otherwise.
     * @throws std::invalid_argument if p is outside [0, 1].
     * @complexity O(1)
     */
    bool bernoulli(double p);

    // ─── Exponential ─────────────────────────────────────────────────────────

    /**
     * @brief Samples from an Exponential distribution Exp(rate).
     *
     * Models inter-arrival times for a Poisson process with the given
     * arrival rate.  Mean = 1/rate.
     *
     * @param rate  Rate parameter λ > 0 (events per tick).
     * @return  Non-negative inter-arrival time sample.
     * @throws std::invalid_argument if rate <= 0.
     * @complexity O(1)
     */
    double exponential(double rate);

    // ─── Normal ──────────────────────────────────────────────────────────────

    /**
     * @brief Samples from a Normal (Gaussian) distribution N(mean, stddev).
     * @param mean    Distribution mean.
     * @param stddev  Standard deviation (must be > 0).
     * @return Sample from N(mean, stddev).
     * @throws std::invalid_argument if stddev <= 0.
     * @complexity O(1)
     */
    double normal(double mean, double stddev);

    // ─── Pareto ──────────────────────────────────────────────────────────────

    /**
     * @brief Samples from a Pareto distribution Pareto(scale, shape).
     *
     * CDF: F(x) = 1 − (scale/x)^shape for x ≥ scale.
     * Mean: shape·scale / (shape − 1) for shape > 1.
     *
     * @param scale  Minimum value x_m > 0.
     * @param shape  Shape parameter α > 0 (tail heaviness).
     * @return Sample ≥ scale.
     * @throws std::invalid_argument if scale <= 0 or shape <= 0.
     * @complexity O(1)
     */
    double pareto(double scale, double shape);

    // ─── Poisson count ───────────────────────────────────────────────────────

    /**
     * @brief Samples the number of Poisson arrivals in one tick.
     *
     * For mean < 30, uses the exact Knuth algorithm. For mean ≥ 30,
     * uses a normal approximation (faster and accurate enough).
     *
     * @param mean  Expected number of arrivals (λ ≥ 0).
     * @return Non-negative integer count.
     * @throws std::invalid_argument if mean < 0.
     * @complexity O(mean) exact; O(1) approximate.
     */
    uint64_t poissonCount(double mean);

    // ─── Engine access ───────────────────────────────────────────────────────

    /**
     * @brief Returns a reference to the underlying Mersenne Twister engine.
     *
     * Use this to construct std::distribution objects in workload classes
     * without duplicating state.
     *
     * @return Reference to std::mt19937_64.
     */
    std::mt19937_64& engine() noexcept;

private:
    uint64_t       seed_;
    std::mt19937_64 engine_;
};

} // namespace embi
