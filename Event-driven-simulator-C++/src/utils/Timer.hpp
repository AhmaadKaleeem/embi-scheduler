/**
 * @file Timer.hpp
 * @brief High-resolution wall-clock timer for performance measurement.
 *
 * Header-only RAII timer using std::chrono::high_resolution_clock.
 * Used to measure scheduler decision latency and simulation wall time.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#pragma once

#include <chrono>

namespace embi {

/**
 * @class Timer
 * @brief RAII wall-clock timer with nanosecond resolution.
 *
 * Starts automatically on construction. Call elapsed_ns() / elapsed_ms() /
 * elapsed_s() to query elapsed time.  reset() restarts the clock.
 *
 * @par Example
 * @code
 * embi::Timer t;
 * do_work();
 * double ns = t.elapsed_ns();
 * @endcode
 */
class Timer {
public:
    using Clock     = std::chrono::high_resolution_clock;
    using TimePoint = Clock::time_point;

    // ─── Construction ────────────────────────────────────────────────────────

    /**
     * @brief Constructs and starts the timer.
     * @complexity O(1)
     */
    Timer() noexcept : start_(Clock::now()) {}

    // ─── Queries ─────────────────────────────────────────────────────────────

    /**
     * @brief Elapsed time in nanoseconds since construction or last reset().
     * @return Elapsed nanoseconds as double.
     * @complexity O(1)
     */
    [[nodiscard]] double elapsed_ns() const noexcept {
        auto end = Clock::now();
        return static_cast<double>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(end - start_).count());
    }

    /**
     * @brief Elapsed time in milliseconds.
     * @return Elapsed milliseconds as double.
     * @complexity O(1)
     */
    [[nodiscard]] double elapsed_ms() const noexcept {
        return elapsed_ns() * 1e-6;
    }

    /**
     * @brief Elapsed time in seconds.
     * @return Elapsed seconds as double.
     * @complexity O(1)
     */
    [[nodiscard]] double elapsed_s() const noexcept {
        return elapsed_ns() * 1e-9;
    }

    // ─── Mutation ────────────────────────────────────────────────────────────

    /**
     * @brief Resets the timer to the current wall-clock time.
     * @complexity O(1)
     */
    void reset() noexcept {
        start_ = Clock::now();
    }

    /**
     * @brief Returns elapsed time and resets the timer (lap behaviour).
     * @return Elapsed nanoseconds since last reset or construction.
     * @complexity O(1)
     */
    double lap_ns() noexcept {
        double ns = elapsed_ns();
        reset();
        return ns;
    }

private:
    TimePoint start_;
};

} // namespace embi
