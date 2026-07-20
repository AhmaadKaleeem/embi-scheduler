/**
 * @file EventQueue.hpp
 * @brief Priority-queue wrapper for discrete-event simulation events.
 *
 * EventQueue maintains a min-heap ordered first by timestamp, then by
 * EventType (Arrival < Schedule < Service < Metrics). This guarantees causal
 * correctness within every simulation tick.
 *
 * The underlying container is pre-reserved to avoid heap allocations in
 * the steady-state simulation loop.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#pragma once

#include "core/Event.hpp"

#include <cstddef>
#include <queue>
#include <vector>

namespace embi {

/**
 * @class EventQueue
 * @brief Min-heap priority queue for simulation events.
 *
 * Events are popped in ascending timestamp order. Within the same timestamp,
 * EventType order is respected (lower ordinal first).
 *
 * @par Complexity
 * - push()  : O(log n)
 * - pop()   : O(log n)
 * - top()   : O(1)
 * - empty() : O(1)
 * - size()  : O(1)
 *
 * @par Memory
 * No heap allocation after the initial reserve() if the queue never grows
 * beyond the reserved capacity.
 *
 * @par Example
 * @code
 * embi::EventQueue q(1024);
 * q.push(makeArrivalEvent(0.0, 3));
 * q.push(makeScheduleEvent(0.0));
 * while (!q.empty()) {
 *     auto e = q.pop();
 *     // process e ...
 * }
 * @endcode
 */
class EventQueue {
public:
    // ─── Construction ─────────────────────────────────────────────────────────

    /**
     * @brief Constructs an empty EventQueue and reserves backing storage.
     * @param initial_capacity  Number of events to pre-allocate for (default 4096).
     *                          Choose ≥ (num_processes + 3) × expected_burst.
     * @complexity O(capacity)
     */
    explicit EventQueue(std::size_t initial_capacity = 4096);

    // ─── Mutating operations ──────────────────────────────────────────────────

    /**
     * @brief Inserts an event into the priority queue.
     * @param event  Event to insert (copied into the queue).
     * @complexity O(log n)
     */
    void push(Event event);

    /**
     * @brief Removes and returns the event with the smallest timestamp.
     *        Within equal timestamps, the event with the lowest EventType fires first.
     * @return The top event.
     * @pre !empty()
     * @throws std::runtime_error if the queue is empty.
     * @complexity O(log n)
     */
    Event pop();

    /**
     * @brief Removes all events from the queue.
     * @post empty() == true
     * @complexity O(n)
     */
    void clear();

    // ─── Read-only operations ─────────────────────────────────────────────────

    /**
     * @brief Returns a reference to the top (minimum) event without removing it.
     * @pre !empty()
     * @throws std::runtime_error if the queue is empty.
     * @complexity O(1)
     */
    [[nodiscard]] const Event& top() const;

    /**
     * @brief Returns true if the queue contains no events.
     * @complexity O(1)
     */
    [[nodiscard]] bool empty() const noexcept;

    /**
     * @brief Returns the number of events currently in the queue.
     * @complexity O(1)
     */
    [[nodiscard]] std::size_t size() const noexcept;

    /**
     * @brief Returns the reserved capacity of the underlying vector.
     * @complexity O(1)
     */
    [[nodiscard]] std::size_t capacity() const noexcept;

private:
    // std::priority_queue is a max-heap by default; EventComparator inverts
    // the ordering, yielding a min-heap.
    using Heap = std::priority_queue<Event, std::vector<Event>, EventComparator>;

    Heap        heap_;
    std::size_t reserved_capacity_;
};

} // namespace embi
