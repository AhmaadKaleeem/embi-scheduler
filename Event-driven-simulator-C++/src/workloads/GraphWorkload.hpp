/**
 * @file GraphWorkload.hpp
 * @brief Generates synthetic dependency topologies for validation.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#pragma once

#include "trace/CanonicalTraceRecord.hpp"
#include "trace/TraceReconstructor.hpp"
#include <vector>

namespace embi {

enum class GraphTopology {
    FanOut,
    FanIn,
    LongChain,
    Diamond,
    Disconnected,
    Cyclic,
    LockConvoy
};

/**
 * @class GraphWorkload
 * @brief Generates pre-defined trace topologies to test the graph scheduler.
 */
class GraphWorkload {
public:
    static std::vector<CanonicalTraceRecord> generate(GraphTopology topo, uint64_t base_tick, uint64_t trace_id);
};

} // namespace embi
