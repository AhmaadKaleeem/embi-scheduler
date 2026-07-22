/**
 * @file GraphState.hpp
 * @brief Dynamic and static graph topologies representing the global simulation state.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#pragma once

#include "trace/TraceReconstructor.hpp"
#include <unordered_set>

namespace embi {

/**
 * @struct GraphState
 * @brief Combines statically reconstructed topologies with dynamically evolving execution state.
 */
struct GraphState {
    // ─── Static Topology (Reconstructed from Trace) ──────────────────────────
    const ReconstructedGraphState* topology{nullptr};

    // ─── Dynamic Execution State ─────────────────────────────────────────────
    std::unordered_set<std::size_t> running_jobs;   ///< PIDs currently executing on a CPU
    std::unordered_set<std::size_t> blocked_jobs;   ///< PIDs currently blocked on a lock or dependency
};

} // namespace embi
