/**
 * @file TraceAnalyzer.cpp
 * @brief Implementation of the trace analyzer.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#include "trace/TraceAnalyzer.hpp"
#include <algorithm>

namespace embi {

TraceStatistics TraceAnalyzer::analyze(const std::vector<CanonicalTraceRecord>& records) {
    TraceStatistics stats;
    if (records.empty()) return stats;

    stats.total_events = records.size();
    
    uint64_t min_tick = records.front().tick;
    uint64_t max_tick = records.back().tick;
    stats.duration_ticks = max_tick - min_tick;
    
    std::unordered_map<uint32_t, uint64_t> arrival_counts;
    std::unordered_map<uint32_t, uint64_t> latency_sum;
    std::unordered_map<uint32_t, uint64_t> latency_count;
    
    uint64_t total_arrivals = 0;
    
    for (const auto& rec : records) {
        if (rec.type == 1) { // Arrival
            arrival_counts[rec.destination_service]++;
            total_arrivals++;
        }
        if (rec.latency > 0) {
            latency_sum[rec.destination_service] += rec.latency;
            latency_count[rec.destination_service]++;
        }
    }
    
    if (stats.duration_ticks > 0) {
        stats.global_arrival_rate = static_cast<double>(total_arrivals) / stats.duration_ticks;
        
        for (const auto& [svc, count] : arrival_counts) {
            stats.arrival_rate_per_service[svc] = static_cast<double>(count) / stats.duration_ticks;
        }
    }
    
    for (const auto& [svc, sum] : latency_sum) {
        if (latency_count[svc] > 0) {
            stats.mean_latency_per_service[svc] = static_cast<double>(sum) / latency_count[svc];
        }
    }
    
    return stats;
}

} // namespace embi
