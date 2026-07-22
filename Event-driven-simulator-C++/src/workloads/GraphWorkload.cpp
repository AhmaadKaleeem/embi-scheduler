/**
 * @file GraphWorkload.cpp
 * @brief Implementation of synthetic topological graph generators.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#include "workloads/GraphWorkload.hpp"

namespace embi {

std::vector<CanonicalTraceRecord> GraphWorkload::generate(GraphTopology topo, uint64_t base_tick, uint64_t trace_id) {
    std::vector<CanonicalTraceRecord> records;
    
    auto add_event = [&](uint64_t rpc_id, uint32_t src, uint32_t dst, uint32_t lat) {
        CanonicalTraceRecord rec{};
        rec.tick = base_tick; // Keep all arrivals at base_tick for parallel structures
        rec.trace_id = trace_id;
        rec.rpc_id = rpc_id;
        rec.source_service = src;
        rec.destination_service = dst;
        rec.container_id = dst; // 1:1 mapping for simplicity
        rec.latency = lat;
        rec.trace_flags = 0;
        rec.type = 1; // Arrival
        rec.priority = 0;
        records.push_back(rec);
    };

    switch (topo) {
        case GraphTopology::FanOut:
            // Service 0 calls Services 1..10 in parallel
            for (uint32_t i = 1; i <= 10; ++i) {
                add_event(i, 0, i, 10);
            }
            break;
            
        case GraphTopology::FanIn:
            // Services 1..10 all call Service 0
            for (uint32_t i = 1; i <= 10; ++i) {
                add_event(i, i, 0, 10);
            }
            break;
            
        case GraphTopology::LongChain:
            // 0 -> 1 -> 2 -> 3 -> 4 -> 5
            for (uint32_t i = 0; i < 5; ++i) {
                add_event(i+1, i, i+1, 10);
            }
            break;
            
        case GraphTopology::Diamond:
            // 0 -> 1, 0 -> 2, 1 -> 3, 2 -> 3
            add_event(1, 0, 1, 10);
            add_event(2, 0, 2, 10);
            add_event(3, 1, 3, 10);
            add_event(4, 2, 3, 10);
            break;
            
        case GraphTopology::LockConvoy:
            // Multiple independent callers contending on a lock (represented via lock traces later)
            for (uint32_t i = 1; i <= 10; ++i) {
                // Event 2 is LockAcquire, Event 3 is LockRelease
                CanonicalTraceRecord acq{};
                acq.tick = base_tick;
                acq.trace_id = trace_id;
                acq.rpc_id = i;
                acq.source_service = i;
                acq.destination_service = i; // self
                acq.trace_flags = 99; // lock_id 99
                acq.type = 2; // LockAcquire
                records.push_back(acq);
                
                add_event(i*100, i, i, 50); // The work inside the critical section
                
                CanonicalTraceRecord rel{};
                rel.tick = base_tick + 50; 
                rel.trace_id = trace_id;
                rel.rpc_id = i*1000;
                rel.source_service = i;
                rel.destination_service = i;
                rel.trace_flags = 99; // lock_id 99
                rel.type = 3; // LockRelease
                records.push_back(rel);
            }
            break;
            
        default:
            break;
    }
    
    return records;
}

} // namespace embi
