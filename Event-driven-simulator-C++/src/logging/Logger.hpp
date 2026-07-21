/**
 * @file Logger.hpp
 * @brief Abstract logger interface and LogRecord type.
 *
 * All concrete loggers (CSV, Binary, Null) inherit from Logger and implement
 * write() and flush(). StatisticsDatabase owns one Logger instance and
 * routes all output through it.
 *
 * @par Binary record format
 * The binary log uses packed structs with a fixed header:
 *
 *   [4B magic: 0x454D4249 "EMBI"] [2B version: 0x0100]
 *   [1B endianness: 0=little] [1B reserved: 0x00]
 *   [4B record_size: sizeof(BinaryRecord)]
 *   [4B reserved: 0x00000000]
 *   [8B start_unix_timestamp_ns]
 *
 * Followed by packed BinaryRecord structs (one per logged tick per process).
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace embi {

// ─── LogRecord ───────────────────────────────────────────────────────────────

/**
 * @struct LogRecord
 * @brief One row of simulation output: tick × process statistics.
 *
 * This is the canonical in-memory representation. Each Logger implementation
 * serialises it to its target format.
 */
struct LogRecord {
    uint64_t    tick{0};
    std::size_t pid{0};
    int64_t     queue_length{0};
    double      arrival_rate{0.0};   ///< true_arrival_rate
    double      service_rate{0.0};   ///< true_service_rate
    double      lambda_hat{0.0};     ///< EWMA arrival estimate
    double      mu_hat{0.0};         ///< EWMA service estimate
    double      scheduler_score{0.0};///< Final/clipped Score assigned by the scheduler
    double      raw_score{0.0};      ///< Raw unclipped score
    bool        chosen{false};       ///< True if this process was scheduled this tick
    double      waiting_time{0.0};   ///< Per-job waiting time (0 if not chosen)
    double      completion_time{0.0};///< Tick of last job completion
    double      throughput{0.0};     ///< Rolling throughput at this tick
    double      lyapunov_v{0.0};     ///< V(t) = Σ Q²
    double      lyapunov_drift{0.0}; ///< ΔV = V(t) − V(t-1)
    
    // Hybrid specific fields
    double      gap{0.0};
    double      tau{0.0};
    int         branch{0};
};

// ─── Binary header ───────────────────────────────────────────────────────────

/**
 * @struct BinaryLogHeader
 * @brief Fixed-size header at the start of every binary log file.
 *
 * Fields are laid out in little-endian byte order.
 */
#pragma pack(push, 1)
struct BinaryLogHeader {
    uint32_t magic{0x454D4249};      ///< "EMBI"
    uint16_t version{0x0100};        ///< Major=1, Minor=0
    uint8_t  endianness{0};          ///< 0 = little-endian
    uint8_t  reserved0{0};
    uint32_t record_size{0};         ///< sizeof(BinaryRecord)
    uint32_t reserved1{0};
    uint64_t start_timestamp_ns{0};  ///< Wall-clock start time (nanoseconds since epoch)
};

/**
 * @struct BinaryRecord
 * @brief Packed binary representation of one LogRecord.
 *
 * All doubles are IEEE 754 double-precision, little-endian.
 */
struct BinaryRecord {
    uint64_t tick{0};
    uint32_t pid{0};
    int32_t  queue_length{0};
    float    arrival_rate{0.0f};
    float    service_rate{0.0f};
    float    lambda_hat{0.0f};
    float    mu_hat{0.0f};
    float    scheduler_score{0.0f};
    float    raw_score{0.0f};
    uint8_t  chosen{0};
    uint8_t  pad[3]{0, 0, 0};
    float    waiting_time{0.0f};
    float    completion_time{0.0f};
    float    throughput{0.0f};
    float    lyapunov_v{0.0f};
    float    lyapunov_drift{0.0f};
    float    gap{0.0f};
    float    tau{0.0f};
    int32_t  branch{0};
};
#pragma pack(pop)

// ─── Logger ──────────────────────────────────────────────────────────────────

/**
 * @class Logger
 * @brief Abstract log sink: receives LogRecord and serialises to output.
 */
class Logger {
public:
    virtual ~Logger() = default;

    /**
     * @brief Writes one LogRecord to the output stream.
     * @param record  The record to serialise.
     * @complexity Implementation-defined (typically O(1) amortised for buffered)
     */
    virtual void write(const LogRecord& record) = 0;

    /**
     * @brief Flushes any internal buffer to the underlying storage.
     * @complexity O(buffer_size)
     */
    virtual void flush() = 0;

    /**
     * @brief Closes the underlying file / stream. Called at end of simulation.
     * @complexity O(buffer_size)
     */
    virtual void close() = 0;

    /**
     * @brief Returns the logger's human-readable type name.
     * @return e.g., "csv", "binary", "null".
     */
    [[nodiscard]] virtual std::string typeName() const noexcept = 0;
};

} // namespace embi
