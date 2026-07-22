/**
 * @file TraceReconstructor.cpp
 * @brief Implementation of the TraceReconstructor.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#include "trace/TraceReconstructor.hpp"

namespace embi {

TraceReconstructor::TraceReconstructor(DependencyMode mode) : mode_(mode) {}

ReconstructedGraphState TraceReconstructor::reconstruct(const std::vector<CanonicalTraceRecord>& records) {
    ReconstructedGraphState state;
    
    buildServiceGraph(records, state);
    buildLockGraph(records, state);
    buildDependencyGraph(records, state);
    
    return state;
}

void TraceReconstructor::buildServiceGraph(const std::vector<CanonicalTraceRecord>& records, ReconstructedGraphState& state) {
    for (const auto& rec : records) {
        if (rec.source_service != 0) { // Assuming 0 is a null/external caller
            state.service_graph[rec.source_service].insert(rec.destination_service);
        }
    }
}

void TraceReconstructor::buildLockGraph(const std::vector<CanonicalTraceRecord>& records, ReconstructedGraphState& state) {
    for (const auto& rec : records) {
        if (rec.type == 2 || rec.type == 3) { // LockAcquire or LockRelease
            // trace_flags holds the lock_id
            state.lock_graph[rec.trace_flags].insert(rec.destination_service);
        }
    }
}

void TraceReconstructor::buildDependencyGraph(const std::vector<CanonicalTraceRecord>& records, ReconstructedGraphState& state) {
    if (mode_ == DependencyMode::Unknown) {
        return; // No assumptions
    }
    
    if (mode_ == DependencyMode::Explicit) {
        // In Explicit mode, we expect priority or trace_flags to encode parent-child directly
        // Currently, our CanonicalTraceRecord lacks an explicit parent_rpc_id field.
        // For SOSP, we would add this field. For now, we fall back to heuristic.
    }
    
    // Heuristic mode:
    // Assume events belonging to the same trace_id are sequentially dependent if they 
    // are ordered in time. E.g., if A arrives before B in the same trace, A -> B.
    // However, parallel fan-outs have same or very close arrival ticks.
    // To be perfectly safe against false serializations, we currently build a simple
    // chain where each RPC depends on the previously seen RPC in the same trace.
    // (This is a naive heuristic that will be improved in Milestone 4 for Fan-outs).
    
    std::unordered_map<uint64_t, uint64_t> last_rpc_for_trace;
    
    for (const auto& rec : records) {
        if (rec.trace_id == 0) continue; // Not part of a trace
        
        auto it = last_rpc_for_trace.find(rec.trace_id);
        if (it != last_rpc_for_trace.end()) {
            uint64_t parent_rpc = it->second;
            if (parent_rpc != rec.rpc_id) {
                state.trace_dags[rec.trace_id][parent_rpc].push_back(rec.rpc_id);
            }
        }
        last_rpc_for_trace[rec.trace_id] = rec.rpc_id;
    }
}

} // namespace embi
