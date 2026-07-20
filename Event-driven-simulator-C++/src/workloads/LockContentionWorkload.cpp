/**
 * @file LockContentionWorkload.cpp
 * @brief Implementation of LockContentionWorkload.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#include "workloads/LockContentionWorkload.hpp"

#include <stdexcept>

namespace embi {

LockContentionWorkload::LockContentionWorkload(uint64_t    seed,
                                               std::size_t num_locks,
                                               std::size_t num_processes,
                                               double      lock_request_rate,
                                               double      hold_mean_ticks)
    : rng_(seed)
    , num_locks_(num_locks)
    , num_processes_(num_processes)
    , request_rate_(lock_request_rate)
    , hold_mean_(hold_mean_ticks)
{
    if (num_locks == 0) {
        throw std::invalid_argument("LockContentionWorkload: num_locks must be >= 1");
    }
    if (lock_request_rate <= 0.0 || lock_request_rate > 1.0) {
        throw std::invalid_argument(
            "LockContentionWorkload: lock_request_rate must be in (0, 1]");
    }
    if (hold_mean_ticks <= 0.0) {
        throw std::invalid_argument(
            "LockContentionWorkload: hold_mean_ticks must be > 0");
    }
}

double LockContentionWorkload::next() {
    // Exponential inter-arrival with rate = request_rate_.
    return rng_.exponential(request_rate_);
}

void LockContentionWorkload::seed(uint64_t s) {
    rng_.reseed(s);
}

std::string_view LockContentionWorkload::name() const noexcept {
    return "lock_contention";
}

double LockContentionWorkload::mean() const noexcept {
    return 1.0 / request_rate_;
}

double LockContentionWorkload::variance() const noexcept {
    return 1.0 / (request_rate_ * request_rate_);
}

std::size_t LockContentionWorkload::numLocks() const noexcept {
    return num_locks_;
}

std::size_t LockContentionWorkload::numProcesses() const noexcept {
    return num_processes_;
}

double LockContentionWorkload::holdDuration() {
    // Exponential with mean = hold_mean_ → rate = 1/hold_mean_.
    double rate = 1.0 / hold_mean_;
    double sample = rng_.exponential(rate);
    // Clamp to at least 1.0 so a hold always requires at least one scheduled tick.
    return sample < 1.0 ? 1.0 : sample;
}

std::size_t LockContentionWorkload::randomLockId() {
    // Uniform integer in [0, num_locks_).
    return static_cast<std::size_t>(rng_.uniformInt(0, static_cast<int64_t>(num_locks_) - 1));
}

} // namespace embi
