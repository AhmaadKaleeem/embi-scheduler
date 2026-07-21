/**
 * @file CanonicalTraceRecord.hpp
 * @brief Schema for the universal trace data format.
 *
 * All datasets (Alibaba, Google, etc.) must be preprocessed into this
 * format before the EventLoop can replay them.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#pragma once

#include <cstdint>

namespace embi {

/**
 * @struct CanonicalTraceRecord
 * @brief 40-byte cache-optimized trace record.
 *
 * Fits cleanly into a cache line, maximizing replay throughput.
 * Contains explicit trace/rpc metadata to support advanced metrics tracking.
 */
#pragma pack(push, 1)
struct CanonicalTraceRecord {
    uint64_t tick;                 ///< Absolute timestamp in simulation ticks
    uint64_t trace_id;             ///< Distributed trace ID
    uint64_t rpc_id;               ///< Individual RPC span ID
    uint32_t source_service;       ///< ID of the calling microservice
    uint32_t destination_service;  ///< ID of the receiving microservice
    uint32_t container_id;         ///< ID of the container/pod
    uint32_t latency;              ///< Expected service latency in ticks
    uint16_t trace_flags;          ///< Bitmask for metadata (e.g., error flags)
    uint8_t  type;                 ///< 1=Arrival, 2=LockAcquire, 3=LockRelease, 4=Completion
    uint8_t  priority;             ///< Tie-breaker priority for simultaneous events
};
#pragma pack(pop)

static_assert(sizeof(CanonicalTraceRecord) == 44, "CanonicalTraceRecord size must be exactly 44 bytes with pack(1)");

} // namespace embi
