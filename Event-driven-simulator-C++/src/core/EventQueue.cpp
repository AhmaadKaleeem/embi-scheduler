/**
 * @file EventQueue.cpp
 * @brief Implementation of EventQueue priority queue operations.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#include "core/EventQueue.hpp"

#include <stdexcept>

namespace embi {

EventQueue::EventQueue(std::size_t initial_capacity)
    : reserved_capacity_(initial_capacity)
{
    // std::priority_queue does not expose reserve() directly.
    // We build it over an internal vector that we can pre-size via
    // a constructor that accepts a container argument.
    // Workaround: construct with a pre-reserved vector.
    std::vector<Event> container;
    container.reserve(initial_capacity);
    heap_ = Heap(EventComparator{}, std::move(container));
}

void EventQueue::push(Event event) {
    heap_.push(std::move(event));
}

Event EventQueue::pop() {
    if (heap_.empty()) {
        throw std::runtime_error("EventQueue::pop: queue is empty");
    }
    Event top = heap_.top();
    heap_.pop();
    return top;
}

void EventQueue::clear() {
    // std::priority_queue has no clear(); rebuild from empty vector.
    std::vector<Event> container;
    container.reserve(reserved_capacity_);
    heap_ = Heap(EventComparator{}, std::move(container));
}

const Event& EventQueue::top() const {
    if (heap_.empty()) {
        throw std::runtime_error("EventQueue::top: queue is empty");
    }
    return heap_.top();
}

bool EventQueue::empty() const noexcept {
    return heap_.empty();
}

std::size_t EventQueue::size() const noexcept {
    return heap_.size();
}

std::size_t EventQueue::capacity() const noexcept {
    return reserved_capacity_;
}

} // namespace embi
