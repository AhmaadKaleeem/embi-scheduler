/**
 * @file NullLogger.hpp
 * @brief Zero-overhead logger that discards all records.
 *
 * Use NullLogger when running maximum-speed benchmarks where detailed
 * per-tick output is unnecessary. write() compiles to a no-op.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#pragma once

#include "logging/Logger.hpp"

namespace embi {

/**
 * @class NullLogger
 * @brief Header-only logger that silently discards all writes.
 *
 * @complexity write() O(1) — zero I/O overhead.
 */
class NullLogger final : public Logger {
public:
    NullLogger() = default;

    void write(const LogRecord& /*record*/) override {}
    void flush()                            override {}
    void close()                            override {}

    [[nodiscard]] std::string typeName() const noexcept override {
        return "null";
    }
};

} // namespace embi
