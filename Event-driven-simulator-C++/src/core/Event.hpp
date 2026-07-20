/**
 * @file Event.hpp
 * @brief Typed simulation event and event-type ordering for the EventQueue.
 *
 * Events are the fundamental currency of the discrete-event simulation engine.
 * Within any given tick, events are ordered by type so that arrivals always
 * precede scheduling, service always precedes metrics collection, and
 * MetricsEvents always fire last.
 *
 * Ordering within same timestamp:
 *   Arrival(0) < Schedule(1) < Service(2) < Metrics(3)
 *
 * This invariant ensures causality: a job cannot be scheduled before it arrives,
 * and metrics cannot be collected before the scheduling decision is applied.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#pragma once

#include <cstddef>
#include <cstdint>

namespace embi {

// ─── Event types ─────────────────────────────────────────────────────────────

/**
 * @enum EventType
 * @brief Discriminates between the four categories of simulation events.
 *
 * The underlying integer value doubles as the within-tick sort key:
 * lower values are processed first.
 */
enum class EventType : uint8_t {
    Arrival  = 0,  ///< A job arrives at a process queue.
    Schedule = 1,  ///< The scheduler evaluates all processes and selects one.
    Service  = 2,  ///< The chosen process completes one job.
    Metrics  = 3,  ///< Online metrics are sampled and a log record is emitted.
};

// ─── Event ───────────────────────────────────────────────────────────────────

/**
 * @struct Event
 * @brief A single discrete-event simulation event.
 *
 * Events are stored in a min-heap (EventQueue) ordered first by timestamp,
 * then by EventType. The payload field carries event-specific data:
 *
 * | EventType | pid        | payload                         |
 * |-----------|------------|---------------------------------|
 * | Arrival   | process id | reserved (0.0)                  |
 * | Schedule  | 0          | reserved (0.0)                  |
 * | Service   | process id | reserved (0.0)                  |
 * | Metrics   | 0          | reserved (0.0)                  |
 *
 * @note Keeping Event a trivially copyable POD type allows the EventQueue's
 *       std::vector backing store to use memcpy during reallocation, which
 *       is important for cache efficiency during large simulations.
 */
struct Event {
    double    timestamp{0.0};          ///< Simulation time at which the event fires.
    EventType type{EventType::Arrival}; ///< Event category.
    std::size_t pid{0};                ///< Target process ID (0 for global events).
    double    payload{0.0};            ///< Event-specific data (see table above).

    // ─── Comparison operators ────────────────────────────────────────────────

    /**
     * @brief Returns true if this event should be processed AFTER other.
     *
     * Used by std::priority_queue (max-heap by default) to build a min-heap:
     * the EventQueue template passes EventComparator which inverts this.
     */
    [[nodiscard]] bool operator>(const Event& other) const noexcept {
        if (timestamp != other.timestamp) {
            return timestamp > other.timestamp;
        }
        return static_cast<uint8_t>(type) > static_cast<uint8_t>(other.type);
    }

    [[nodiscard]] bool operator<(const Event& other) const noexcept {
        return other > *this;
    }

    [[nodiscard]] bool operator==(const Event& other) const noexcept {
        return timestamp == other.timestamp && type == other.type && pid == other.pid;
    }
};

// ─── EventComparator ─────────────────────────────────────────────────────────

/**
 * @struct EventComparator
 * @brief Strict-weak-ordering comparator for std::priority_queue min-heap.
 *
 * std::priority_queue is a max-heap by default. Passing EventComparator
 * inverts the ordering, yielding a min-heap where the event with the
 * smallest timestamp (and then lowest EventType value) is at the top.
 */
struct EventComparator {
    [[nodiscard]] bool operator()(const Event& a, const Event& b) const noexcept {
        return a > b;  // delegate to Event::operator>
    }
};

// ─── Factory helpers ─────────────────────────────────────────────────────────

/// Constructs an ArrivalEvent for the given process at the given tick.
[[nodiscard]] inline Event makeArrivalEvent(double tick, std::size_t pid) noexcept {
    return Event{tick, EventType::Arrival, pid, 0.0};
}

/// Constructs a ScheduleEvent for the given tick.
[[nodiscard]] inline Event makeScheduleEvent(double tick) noexcept {
    return Event{tick, EventType::Schedule, 0, 0.0};
}

/// Constructs a ServiceEvent for the chosen process at the given tick.
[[nodiscard]] inline Event makeServiceEvent(double tick, std::size_t pid) noexcept {
    return Event{tick, EventType::Service, pid, 0.0};
}

/// Constructs a MetricsEvent for the given tick.
[[nodiscard]] inline Event makeMetricsEvent(double tick) noexcept {
    return Event{tick, EventType::Metrics, 0, 0.0};
}

} // namespace embi
