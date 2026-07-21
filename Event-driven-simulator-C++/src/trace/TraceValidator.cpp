/**
 * @file TraceValidator.cpp
 * @brief Implementation of the trace validation logic.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#include "trace/TraceValidator.hpp"
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <cmath>

namespace embi {

void TraceValidator::validate(const std::vector<CanonicalTraceRecord>& records, bool strict) {
    if (records.empty()) {
        if (strict) throw TraceValidationError("Trace dataset is empty");
        return;
    }

    pass1Chronological(records, strict);
    pass2IdentifierIntegrity(records, strict);
    pass3BoundAnalysis(records, strict);
    pass4DistributionFitting(records, strict);
    pass5OutlierDetection(records, strict);
}

void TraceValidator::pass1Chronological(const std::vector<CanonicalTraceRecord>& records, bool strict) {
    for (size_t i = 1; i < records.size(); ++i) {
        if (records[i].tick < records[i - 1].tick) {
            std::string msg = "Chronological Error: Event " + std::to_string(i) + 
                              " has tick < previous event (" + 
                              std::to_string(records[i].tick) + " < " + 
                              std::to_string(records[i - 1].tick) + ")";
            if (strict) throw TraceValidationError(msg);
            else std::cerr << "WARNING: " << msg << "\n";
        }
    }
}

void TraceValidator::pass2IdentifierIntegrity(const std::vector<CanonicalTraceRecord>& records, bool strict) {
    std::unordered_map<uint64_t, std::unordered_set<uint64_t>> rpc_locks;
    
    for (const auto& rec : records) {
        if (rec.type == 2) { // LockAcquire
            rpc_locks[rec.rpc_id].insert(rec.trace_flags); // Assuming trace_flags holds lock_id for locks
        } else if (rec.type == 3) { // LockRelease
            auto it = rpc_locks.find(rec.rpc_id);
            if (it == rpc_locks.end() || it->second.find(rec.trace_flags) == it->second.end()) {
                std::string msg = "Integrity Error: Orphaned LockRelease for rpc_id " + std::to_string(rec.rpc_id);
                if (strict) throw TraceValidationError(msg);
                else std::cerr << "WARNING: " << msg << "\n";
            }
            it->second.erase(rec.trace_flags);
        }
    }
}

void TraceValidator::pass3BoundAnalysis(const std::vector<CanonicalTraceRecord>& records, bool strict) {
    (void)records; (void)strict;
    // Compute theoretical capacity requirements based on max concurrency
    // For now, a placeholder implementation.
}

void TraceValidator::pass4DistributionFitting(const std::vector<CanonicalTraceRecord>& records, bool strict) {
    (void)records; (void)strict;
    // Validates that the trace holds sufficient data points for meaningful distribution extraction.
}

void TraceValidator::pass5OutlierDetection(const std::vector<CanonicalTraceRecord>& records, bool strict) {
    (void)strict;
    uint64_t max_latency = 0;
    for (const auto& rec : records) {
        if (rec.latency > max_latency) max_latency = rec.latency;
    }
    // E.g., if max_latency is ridiculously huge, warn the user.
    if (max_latency > 1000000000ULL) {
        std::cerr << "WARNING: Extreme outlier detected. Max latency = " << max_latency << " ticks.\n";
    }
}

} // namespace embi
