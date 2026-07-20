/**
 * @file CSVLogger.hpp
 * @brief Buffered CSV logger writing simulation records to disk.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#pragma once

#include "logging/Logger.hpp"

#include <fstream>
#include <string>
#include <vector>

namespace embi {

/**
 * @class CSVLogger
 * @brief Buffered CSV sink that writes LogRecord rows to a .csv file.
 *
 * Rows are buffered in memory and flushed to disk either when the buffer
 * fills up or when flush() / close() is called explicitly. The buffer
 * size defaults to 64 KB of rows.
 *
 * CSV header:
 *   tick,pid,queue,arrival,service,lambda_hat,mu_hat,scheduler_score,
 *   chosen,waiting_time,completion_time,throughput,V,drift
 *
 * @par Example
 * @code
 * embi::CSVLogger logger("output/run_0.csv", 4096);
 * logger.write(record);
 * logger.close();
 * @endcode
 */
class CSVLogger final : public Logger {
public:
    /// Default number of records to buffer before flushing.
    static constexpr std::size_t kDefaultBufferRecords = 4096;

    /**
     * @brief Opens the output file and writes the CSV header.
     * @param path            Output file path (created / truncated).
     * @param buffer_records  Number of records to buffer before auto-flush.
     * @throws std::runtime_error if the file cannot be opened.
     */
    explicit CSVLogger(const std::string& path,
                       std::size_t buffer_records = kDefaultBufferRecords);

    ~CSVLogger() override;

    void write(const LogRecord& record) override;
    void flush()                        override;
    void close()                        override;

    [[nodiscard]] std::string typeName() const noexcept override { return "csv"; }

    /**
     * @brief Returns the output file path.
     * @complexity O(1)
     */
    [[nodiscard]] const std::string& path() const noexcept;

    /**
     * @brief Returns total records written (including buffered).
     * @complexity O(1)
     */
    [[nodiscard]] uint64_t recordsWritten() const noexcept;

private:
    void flushBuffer();
    void writeHeader();

    std::string         path_;
    std::ofstream       file_;
    std::vector<std::string> buffer_;  ///< Pre-formatted CSV row strings
    std::size_t         buffer_cap_;
    uint64_t            records_written_{0};
    bool                closed_{false};
};

} // namespace embi
