/**
 * @file Statistics.hpp
 * @brief Lightweight statistics accumulator for aggregate run-level metrics.
 *
 * Statistics collects per-run scalar values (throughput, mean wait, etc.)
 * across multiple seeds or arrival rates in an experiment sweep, then
 * exports mean ± std dev for each metric.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace embi {

/**
 * @class Statistics
 * @brief Online accumulator for experiment-level aggregate statistics.
 *
 * @par Example
 * @code
 * embi::Statistics stats;
 * stats.record("jain", 0.92);
 * stats.record("jain", 0.88);
 * auto [mean, std] = stats.meanStdDev("jain");
 * @endcode
 */
class Statistics {
public:
    /**
     * @brief Records one sample for a named metric.
     * @param name   Metric name (e.g., "avg_wait", "jain").
     * @param value  Scalar sample value.
     * @complexity O(1) amortised
     */
    void record(const std::string& name, double value);

    /**
     * @brief Returns mean and population std dev for a metric.
     * @param name  Metric name.
     * @return {mean, std_dev}. Returns {0, 0} if no samples.
     * @complexity O(n_samples)
     */
    [[nodiscard]] std::pair<double, double> meanStdDev(const std::string& name) const;

    /**
     * @brief Returns all samples for a given metric.
     * @param name  Metric name.
     * @return Const reference to sample vector. Empty if no samples.
     * @complexity O(1)
     */
    [[nodiscard]] const std::vector<double>& samples(const std::string& name) const;

    /**
     * @brief Returns all metric names recorded.
     * @complexity O(n_metrics)
     */
    [[nodiscard]] std::vector<std::string> metricNames() const;

    /**
     * @brief Returns total number of samples across all metrics.
     * @complexity O(n_metrics)
     */
    [[nodiscard]] std::size_t totalSamples() const noexcept;

    /**
     * @brief Clears all recorded samples.
     * @complexity O(n_metrics)
     */
    void reset();

private:
    std::unordered_map<std::string, std::vector<double>> data_;
    static const std::vector<double> kEmpty_;
};

} // namespace embi
