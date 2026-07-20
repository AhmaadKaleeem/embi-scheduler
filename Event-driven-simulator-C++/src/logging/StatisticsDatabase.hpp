/**
 * @file StatisticsDatabase.hpp
 * @brief Unified output management: owns the logger and routes all writes.
 *
 * StatisticsDatabase is the single public interface for simulation output.
 * It:
 *   - Owns the Logger (CSV, Binary, or Null)
 *   - Routes write(tick, pid, ...) through the Logger
 *   - Accumulates aggregate statistics via Statistics
 *   - Exports CSV, Binary, and JSON summaries via SummaryWriter
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#pragma once

#include "logging/Logger.hpp"
#include "logging/Statistics.hpp"
#include "core/Config.hpp"
#include "core/OfflineMetrics.hpp"
#include "core/OnlineMetrics.hpp"

#include <memory>
#include <string>

namespace embi {

/**
 * @class StatisticsDatabase
 * @brief Unified simulation output manager.
 *
 * @par Usage
 * @code
 * auto db = embi::StatisticsDatabase::create(config);
 * db->record(record);           // route to logger
 * db->flush();                  // flush logger
 * db->writeSummaryTxt(path);    // write summary.txt
 * db->exportJSONSummary(path);  // write summary.json
 * @endcode
 */
class StatisticsDatabase {
public:
    /**
     * @brief Factory: creates the appropriate Logger based on config.
     *
     * - config.null_log   → NullLogger
     * - config.binary_log → BinaryLogger at output_dir/run.bin
     * - default           → CSVLogger   at output_dir/run.csv
     *
     * Creates output_dir if it does not exist.
     *
     * @param config  Simulation configuration.
     * @return Owning unique_ptr to a StatisticsDatabase.
     * @throws std::runtime_error if file creation fails.
     */
    [[nodiscard]] static std::unique_ptr<StatisticsDatabase>
    create(const Config& config);

    /**
     * @brief Constructs a StatisticsDatabase with an injected Logger.
     * @param logger  Owning pointer to any Logger implementation.
     * @param config  Simulation configuration.
     */
    StatisticsDatabase(std::unique_ptr<Logger> logger, const Config& config);

    // ─── Core output ──────────────────────────────────────────────────────────

    /**
     * @brief Routes a LogRecord to the underlying Logger.
     * @param record  The record to write.
     * @complexity O(1) amortised (buffered)
     */
    void record(const LogRecord& record);

    /**
     * @brief Flushes the underlying logger's buffer to disk.
     * @complexity O(buffer_size)
     */
    void flush();

    /**
     * @brief Closes the underlying logger (writes remaining buffer).
     * @complexity O(buffer_size)
     */
    void close();

    // ─── Aggregate statistics ─────────────────────────────────────────────────

    /**
     * @brief Records a scalar sample into the aggregate Statistics store.
     * @param name   Metric name.
     * @param value  Sample value.
     */
    void recordStat(const std::string& name, double value);

    /**
     * @brief Returns a reference to the internal Statistics store.
     */
    [[nodiscard]] const Statistics& stats() const noexcept;

    // ─── Export ───────────────────────────────────────────────────────────────

    /**
     * @brief Writes a human-readable summary.txt file.
     * @param path            Output path.
     * @param online          Final online metrics snapshot.
     * @param offline_report  Computed offline report.
     * @throws std::runtime_error if file cannot be written.
     */
    void writeSummaryTxt(const std::string&    path,
                          const OnlineSnapshot& online,
                          const OfflineReport&  offline_report) const;

    /**
     * @brief Exports aggregate statistics and offline report as JSON.
     * @param path  Output path.
     * @throws std::runtime_error if file cannot be written.
     */
    void exportJSONSummary(const std::string&    path,
                            const OnlineSnapshot& online,
                            const OfflineReport&  offline_report) const;

    /**
     * @brief Returns the underlying logger's type name.
     */
    [[nodiscard]] std::string loggerType() const noexcept;

private:
    std::unique_ptr<Logger> logger_;
    Statistics              stats_;
    Config                  config_;
};

} // namespace embi
