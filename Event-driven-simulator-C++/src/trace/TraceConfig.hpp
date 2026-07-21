/**
 * @file TraceConfig.hpp
 * @brief Standalone configuration for trace execution.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#pragma once

#include <string>
#include <cstdint>
#include <optional>

namespace embi {

struct TraceConfig {
    std::optional<std::string> trace_file;
    std::string format{"alibaba"};         // e.g., "alibaba", "google"
    uint64_t    tick_resolution{1000};
    bool        strict{true};
    double      time_scale{1.0};           // > 1.0 = Accelerated Replay
    std::string validation_mode{"strict"}; // "strict" or "warning"
    uint64_t    window_begin{0};
    uint64_t    window_end{UINT64_MAX};
    uint64_t    num_processes{16};
};

} // namespace embi
