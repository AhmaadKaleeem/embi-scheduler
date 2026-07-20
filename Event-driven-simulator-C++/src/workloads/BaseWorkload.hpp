/**
 * @file BaseWorkload.hpp
 * @brief Abstract workload interface for inter-arrival time generation.
 *
 * All workload generators inherit from BaseWorkload and implement next(),
 * which returns an inter-arrival time in simulation ticks. The EventLoop
 * maintains a per-process "next arrival tick" counter and calls next()
 * whenever a new arrival needs to be scheduled.
 *
 * @par Statistical Contract
 * Implementations must document:
 *   - Theoretical mean()   of the returned distribution
 *   - Theoretical variance() of the returned distribution
 *
 * These are used in test_workloads.cpp to validate that the workload
 * generates the correct statistical moments over long runs.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#pragma once

#include <cstdint>
#include <string_view>

namespace embi {

/**
 * @class BaseWorkload
 * @brief Pure abstract interface for inter-arrival time generators.
 *
 * @par Threading
 * Workload instances are NOT thread-safe. Each simulation (each Simulator
 * instance in an Experiment sweep) owns its own workload instance.
 */
class BaseWorkload {
public:
    virtual ~BaseWorkload() = default;

    // ─── Core interface ───────────────────────────────────────────────────────

    /**
     * @brief Samples the next inter-arrival time in simulation ticks.
     *
     * The EventLoop calls next() once per arrival for each process.
     * The returned value is the delay until the NEXT arrival, measured
     * from the current arrival.
     *
     * @return Inter-arrival time ≥ 0 (in ticks; may be fractional).
     * @complexity Implementation-defined (typically O(1))
     */
    virtual double next() = 0;

    /**
     * @brief Re-seeds the internal PRNG for deterministic replay.
     * @param seed  64-bit seed value.
     * @complexity O(1)
     */
    virtual void seed(uint64_t s) = 0;

    /**
     * @brief Returns the workload's human-readable name (e.g., "poisson").
     * @complexity O(1)
     */
    [[nodiscard]] virtual std::string_view name() const noexcept = 0;

    // ─── Statistical properties ───────────────────────────────────────────────

    /**
     * @brief Returns the theoretical mean inter-arrival time (1/λ).
     * @complexity O(1)
     */
    [[nodiscard]] virtual double mean() const noexcept = 0;

    /**
     * @brief Returns the theoretical variance of inter-arrival times.
     * @complexity O(1)
     */
    [[nodiscard]] virtual double variance() const noexcept = 0;
};

} // namespace embi
