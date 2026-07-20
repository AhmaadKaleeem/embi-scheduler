/**
 * @file BinaryLogger.hpp
 * @brief High-performance binary log file writer.
 *
 * Writes packed BinaryRecord structs to a binary file with a fixed header.
 * Achieves higher throughput than CSVLogger and produces files that can be
 * memory-mapped and parsed directly by numpy.frombuffer() in Python scripts.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#pragma once

#include "logging/Logger.hpp"

#include <chrono>
#include <fstream>
#include <string>
#include <vector>

namespace embi {

/**
 * @class BinaryLogger
 * @brief Buffered binary logger using packed BinaryRecord structs.
 *
 * The output file layout:
 *   [BinaryLogHeader][BinaryRecord][BinaryRecord]...
 *
 * Python reading example:
 * @code{.py}
 * import numpy as np
 * header_size = 24  # sizeof(BinaryLogHeader)
 * rec_dtype = np.dtype([
 *     ('tick', 'u8'), ('pid', 'u4'), ('queue_length', 'i4'),
 *     ('arrival_rate', 'f4'), ('service_rate', 'f4'),
 *     ('lambda_hat', 'f4'), ('mu_hat', 'f4'),
 *     ('scheduler_score', 'f4'), ('chosen', 'u1'), ('pad', 'V3'),
 *     ('waiting_time', 'f4'), ('completion_time', 'f4'),
 *     ('throughput', 'f4'), ('lyapunov_v', 'f4'), ('lyapunov_drift', 'f4')
 * ])
 * data = np.fromfile('run.bin', dtype=rec_dtype, offset=header_size)
 * @endcode
 */
class BinaryLogger final : public Logger {
public:
    static constexpr std::size_t kDefaultBufferRecords = 8192;

    /**
     * @brief Opens the binary log file and writes the header.
     * @param path            Output file path.
     * @param buffer_records  Number of records to buffer before flush.
     * @throws std::runtime_error if the file cannot be opened.
     */
    explicit BinaryLogger(const std::string& path,
                          std::size_t buffer_records = kDefaultBufferRecords);

    ~BinaryLogger() override;

    void write(const LogRecord& record) override;
    void flush()                        override;
    void close()                        override;

    [[nodiscard]] std::string typeName() const noexcept override { return "binary"; }

    /// Returns total records written.
    [[nodiscard]] uint64_t recordsWritten() const noexcept;

private:
    void writeFileHeader();
    void flushBuffer();

    std::string              path_;
    std::ofstream            file_;
    std::vector<BinaryRecord> buffer_;
    std::size_t              buffer_cap_;
    uint64_t                 records_written_{0};
    bool                     closed_{false};
};

} // namespace embi
