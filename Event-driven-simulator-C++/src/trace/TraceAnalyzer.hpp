/**
 * @file TraceAnalyzer.hpp
 * @brief Pre-simulation statistical analyzer for validated traces.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#pragma once

#include "trace/CanonicalTraceRecord.hpp"
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace embi {

struct TraceStatistics {
    uint64_t total_events{0};
    uint64_t duration_ticks{0};
    double   global_arrival_rate{0.0};
    std::unordered_map<uint32_t, double> arrival_rate_per_service;
    std::unordered_map<uint32_t, double> mean_latency_per_service;
};

class TraceAnalyzer {
public:
    /**
     * @brief Extracts statistical distributions and rates from the trace.
     * @param records A validated vector of canonical records.
     * @return Aggregated statistics for the trace.
     */
    static TraceStatistics analyze(const std::vector<CanonicalTraceRecord>& records);
};

} // namespace embi
