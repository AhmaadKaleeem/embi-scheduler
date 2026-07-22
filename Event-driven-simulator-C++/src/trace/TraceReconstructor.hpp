/**
 * @file TraceReconstructor.hpp
 * @brief Recovers causal edges, locks, and service topologies from traces.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#pragma once

#include "trace/CanonicalTraceRecord.hpp"
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace embi {

enum class DependencyMode {
    Explicit,   // Rely on explicit trace parent/child metadata
    Heuristic,  // Infer DAGs from trace_id and temporal ordering
    Unknown     // Treat all events as independent M/G/1 arrivals
};

struct ReconstructedGraphState {
    // Service Topology Graph: source -> destinations
    std::unordered_map<uint32_t, std::unordered_set<uint32_t>> service_graph;
    
    // Lock Graph: lock_id -> set of accessing services
    std::unordered_map<uint32_t, std::unordered_set<uint32_t>> lock_graph;
    
    // Trace Dependency Graph (DAG): trace_id -> (rpc_id -> children_rpc_ids)
    std::unordered_map<uint64_t, std::unordered_map<uint64_t, std::vector<uint64_t>>> trace_dags;
};

class TraceReconstructor {
public:
    explicit TraceReconstructor(DependencyMode mode = DependencyMode::Heuristic);

    /**
     * @brief Reconstructs the topologies from the parsed trace records.
     * @param records The parsed canonical trace records.
     * @return The recovered global topological state.
     */
    ReconstructedGraphState reconstruct(const std::vector<CanonicalTraceRecord>& records);

private:
    DependencyMode mode_;
    
    void buildServiceGraph(const std::vector<CanonicalTraceRecord>& records, ReconstructedGraphState& state);
    void buildLockGraph(const std::vector<CanonicalTraceRecord>& records, ReconstructedGraphState& state);
    void buildDependencyGraph(const std::vector<CanonicalTraceRecord>& records, ReconstructedGraphState& state);
};

} // namespace embi
