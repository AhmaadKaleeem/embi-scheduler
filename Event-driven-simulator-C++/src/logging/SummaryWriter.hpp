/**
 * @file SummaryWriter.hpp
 * @brief Writes human-readable and machine-readable experiment summaries.
 *
 * SummaryWriter generates the final output files for a completed simulation
 * or experiment sweep:
 *
 *   summary.txt  — human-readable tabular report
 *   summary.json — machine-readable JSON (same data)
 *
 * For experiment sweeps, it produces a combined table comparing all schedulers
 * ranked by Jain Fairness Index, average waiting time, and throughput.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#pragma once

#include "core/OfflineMetrics.hpp"
#include "core/OnlineMetrics.hpp"
#include "core/Config.hpp"

#include <string>
#include <vector>

namespace embi {

/// Lightweight aggregate result record for one simulation run.
struct RunSummary {
    std::string scheduler_name;
    std::string workload_name;
    uint64_t    seed{0};
    double      arrival_rate{0.0};
    std::vector<double> arrival_rate_asymmetric;
    Config      config;
    OnlineSnapshot online;
    OfflineReport  offline;
};

/**
 * @class SummaryWriter
 * @brief Writes experiment summary reports.
 *
 * @par Example
 * @code
 * embi::SummaryWriter sw;
 * sw.writeComparativeTable("results/summary.txt", summaries);
 * @endcode
 */
class SummaryWriter {
public:
    SummaryWriter() = default;

    /**
     * @brief Writes a comparative summary table for multiple scheduler runs.
     *
     * Produces a table with columns:
     *   Scheduler | Workload | ArrRate | AvgWait | P99 | Throughput | Jain | Utilization
     *
     * Rows are sorted by Jain Fairness Index (descending).
     *
     * @param path       Output file path for summary.txt.
     * @param summaries  Vector of per-run RunSummary objects.
     * @throws std::runtime_error if file cannot be written.
     * @complexity O(R log R) where R = number of runs.
     */
    void writeComparativeTable(const std::string&          path,
                                const std::vector<RunSummary>& summaries) const;

    /**
     * @brief Writes a JSON file with all run summaries for Python analysis.
     * @param path       Output JSON path.
     * @param summaries  Per-run summaries.
     * @throws std::runtime_error if file cannot be written.
     */
    void writeJSONSummary(const std::string&          path,
                           const std::vector<RunSummary>& summaries) const;
};

} // namespace embi
