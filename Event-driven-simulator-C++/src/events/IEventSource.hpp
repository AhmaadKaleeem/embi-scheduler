/**
 * @file IEventSource.hpp
 * @brief Universal event generator interface for the EventLoop.
 *
 * IEventSource decouples the core simulation loop from the origin of events.
 * It is implemented by SyntheticEventSource (which uses statistical distributions)
 * and TraceReplaySource (which feeds pre-recorded canonical traces).
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#pragma once

#include "core/EventQueue.hpp"
#include <cstdint>

namespace embi {

/**
 * @class IEventSource
 * @brief Pure virtual interface for generating events at a specific tick.
 */
class IEventSource {
public:
    virtual ~IEventSource() = default;

    /**
     * @brief Pushes any events scheduled for the given tick into the queue.
     * 
     * The EventLoop calls this once per tick before inserting Schedule
     * and Metrics events. The source may push Arrival, LockAcquire, or
     * other domain-specific events.
     *
     * @param tick_d The current simulation tick (as double for precision).
     * @param queue  The central EventQueue to push events into.
     */
    virtual void emitEvents(double tick_d, EventQueue& queue) = 0;
};

} // namespace embi
