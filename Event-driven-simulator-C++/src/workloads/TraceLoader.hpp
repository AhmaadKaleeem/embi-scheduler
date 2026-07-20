/**
 * @file TraceLoader.hpp
 * @brief File-driven trace workload: replays recorded arrival sequences.
 *
 * Reads a CSV trace file and replays arrivals in tick order. Supports
 * wrapping (replay loops when the trace is exhausted) for long simulations.
 *
 * @par Trace File Format
 * CSV with a header row, columns:
 *   tick,pid,arrival,service,priority,deadline
 *
 * - tick     : integer, simulation tick of the arrival event
 * - pid      : integer, destination process ID
 * - arrival  : integer, number of jobs arriving (default 1 if omitted)
 * - service  : float,   service time for these jobs (default 1.0 if omitted)
 * - priority : float,   job priority (optional; ignored if workload doesn't use it)
 * - deadline : float,   absolute deadline (optional; ignored if workload doesn't use it)
 *
 * Lines beginning with '#' are treated as comments.
 * Missing optional columns are silently ignored.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#pragma once

#include "workloads/BaseWorkload.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace embi {

/**
 * @struct TraceRecord
 * @brief One row from a trace file, representing arrivals at a specific tick.
 */
struct TraceRecord {
    uint64_t tick{0};              ///< Arrival tick.
    std::size_t pid{0};            ///< Target process ID.
    uint64_t arrival_count{1};     ///< Number of jobs arriving.
    double   service_time{1.0};    ///< Service duration per job.
    std::optional<double> priority; ///< Job priority (optional).
    std::optional<double> deadline; ///< Absolute deadline (optional).
};

/**
 * @class TraceLoader
 * @brief Trace-driven inter-arrival generator.
 *
 * next() returns the inter-arrival time to the NEXT trace record's tick,
 * advancing through the sorted records. When the trace is exhausted and
 * looping is enabled, it wraps back to the beginning.
 *
 * @par Example
 * @code
 * embi::TraceLoader w("trace.csv", true);  // with looping
 * double inter = w.next();
 * @endcode
 */
class TraceLoader final : public BaseWorkload {
public:
    /**
     * @brief Constructs a TraceLoader by reading and parsing the trace file.
     * @param path     Path to the CSV trace file.
     * @param looping  If true, wrap to beginning when trace exhausted.
     * @throws std::runtime_error if file cannot be opened.
     * @throws std::invalid_argument if file has no valid records.
     * @complexity O(N) where N = number of trace records.
     */
    explicit TraceLoader(const std::string& path, bool looping = true);

    double           next()              override;
    void             seed(uint64_t s)    override;  ///< Resets playback position.
    std::string_view name() const noexcept override;
    double           mean() const noexcept override;
    double           variance() const noexcept override;

    /**
     * @brief Returns the total number of loaded trace records.
     * @complexity O(1)
     */
    [[nodiscard]] std::size_t recordCount() const noexcept;

    /**
     * @brief Returns a reference to all loaded records (read-only).
     * @complexity O(1)
     */
    [[nodiscard]] const std::vector<TraceRecord>& records() const noexcept;

    /**
     * @brief Returns the index of the next record to be replayed.
     * @complexity O(1)
     */
    [[nodiscard]] std::size_t currentIndex() const noexcept;

private:
    std::vector<TraceRecord> records_;
    std::size_t              index_{0};
    double                   last_tick_{0.0};
    bool                     looping_{true};

    // Precomputed statistical moments from the trace
    double mean_{0.0};
    double variance_{0.0};

    /// Parses the CSV file into records_; throws on I/O error.
    void loadFile(const std::string& path);

    /// Computes mean/variance of inter-arrival times from loaded records.
    void computeStatistics();
};

} // namespace embi
