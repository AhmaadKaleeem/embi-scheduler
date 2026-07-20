/**
 * @file LockContentionWorkload.hpp
 * @brief Workload that generates lock-acquisition request inter-arrival times.
 *
 * Unlike other workloads (which model job arrivals at process queues),
 * LockContentionWorkload generates inter-arrival times for lock-acquisition
 * *requests*.  Each call to next() returns the time until the next lock
 * request from a single process.
 *
 * The EventLoop calls next() once per process per pending request to schedule
 * LockAcquireEvents, exactly as it calls next() for ordinary ArrivalEvents.
 *
 * Hold durations are drawn independently via holdDuration().
 *
 * @par Statistical Contract
 *   - mean()     = 1 / lock_request_rate  (mean inter-request time)
 *   - variance() = 1 / lock_request_rate² (exponential distribution)
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#pragma once

#include "workloads/BaseWorkload.hpp"
#include "utils/Random.hpp"

#include <cstddef>
#include <cstdint>

namespace embi {

/**
 * @class LockContentionWorkload
 * @brief Exponential inter-arrival generator for lock-acquisition requests.
 *
 * @par Example
 * @code
 * embi::LockContentionWorkload w(42, 4, 64, 0.3, 5.0);
 * double inter = w.next();       // time until next lock request
 * double hold  = w.holdDuration(); // how many CPU ticks the lock is held
 * @endcode
 */
class LockContentionWorkload final : public BaseWorkload {
public:
    /**
     * @brief Constructs a LockContentionWorkload.
     * @param seed              PRNG seed.
     * @param num_locks         Number of independent locks in the pool.
     * @param num_processes     Number of processes (for lock selection).
     * @param lock_request_rate Poisson rate of lock-acquisition requests
     *                          per process per tick, must be in (0, 1].
     * @param hold_mean_ticks   Mean CPU ticks a process holds a lock
     *                          (Exp distributed), must be > 0.
     * @throws std::invalid_argument on invalid parameters.
     */
    LockContentionWorkload(uint64_t    seed,
                           std::size_t num_locks,
                           std::size_t num_processes,
                           double      lock_request_rate,
                           double      hold_mean_ticks);

    // ─── BaseWorkload interface ───────────────────────────────────────────────

    /// Returns the next inter-request time for one process (Exp(lock_request_rate)).
    double           next()                  override;
    void             seed(uint64_t s)        override;
    [[nodiscard]] std::string_view name()    const noexcept override;
    [[nodiscard]] double mean()              const noexcept override;
    [[nodiscard]] double variance()          const noexcept override;

    // ─── Lock-specific interface ──────────────────────────────────────────────

    /// Returns the number of locks in the pool.
    [[nodiscard]] std::size_t numLocks()    const noexcept;

    /// Returns the number of processes.
    [[nodiscard]] std::size_t numProcesses() const noexcept;

    /**
     * @brief Samples one hold duration from Exp(1 / hold_mean_ticks).
     * @return CPU ticks the holder must accumulate before releasing.
     */
    double holdDuration();

    /**
     * @brief Samples a random lock index in [0, num_locks).
     * @return Lock ID.
     */
    std::size_t randomLockId();

private:
    Random      rng_;
    std::size_t num_locks_;
    std::size_t num_processes_;
    double      request_rate_;   ///< λ: requests per tick per process
    double      hold_mean_;      ///< Mean CPU ticks per hold
};

} // namespace embi
