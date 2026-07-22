/**
 * @file ReplayEngine.cpp
 * @brief Implementation of the trace replay engine.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#include "trace/ReplayEngine.hpp"
#include <cmath>

namespace embi {

ReplayEngine::ReplayEngine(std::shared_ptr<const TraceStore> store, TraceConfig config)
    : store_(std::move(store)), config_(std::move(config)) {}

void ReplayEngine::emitEvents(double tick_d, EventQueue& queue) {
    if (isDone()) return;

    // We assume tick is exactly integers for replay mapping.
    uint64_t current_tick = static_cast<uint64_t>(std::round(tick_d));

    while (current_idx_ < store_->size()) {
        const auto& rec = (*store_)[current_idx_];
        
        // Time scaling mapping
        uint64_t mapped_tick = static_cast<uint64_t>(rec.tick / config_.time_scale);
        
        if (mapped_tick > current_tick) {
            break; // Event is in the future
        }
        
        if (mapped_tick == current_tick) {
            EventType event_type;
            switch (rec.type) {
                case 1: event_type = EventType::Arrival; break;
                case 2: event_type = EventType::LockAcquire; break;
                case 3: event_type = EventType::LockRelease; break;
                case 4: event_type = EventType::Service; break;
                default: 
                    // Skip unknown event types for now
                    current_idx_++;
                    continue;
            }
            double payload = 0.0;
            if (event_type == EventType::Arrival) {
                payload = static_cast<double>(rec.latency);
            } else if (event_type == EventType::LockAcquire || event_type == EventType::LockRelease) {
                payload = static_cast<double>(rec.trace_flags);
            }

            queue.push(Event{
                static_cast<double>(mapped_tick),
                event_type,
                static_cast<std::size_t>(rec.destination_service),
                payload,
                rec.trace_id,
                rec.rpc_id,
                static_cast<std::size_t>(rec.source_service),
                rec.priority
            });
        }
        current_idx_++;
    }
}

} // namespace embi
