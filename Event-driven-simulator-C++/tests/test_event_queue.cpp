/**
 * @file test_event_queue.cpp
 * @brief Unit tests for EventQueue ordering invariants.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#include <gtest/gtest.h>
#include "core/EventQueue.hpp"
#include "core/Event.hpp"

namespace embi {

TEST(EventQueueTest, EmptyOnConstruction) {
    EventQueue q;
    EXPECT_TRUE(q.empty());
    EXPECT_EQ(q.size(), 0UL);
}

TEST(EventQueueTest, PushIncreasesSize) {
    EventQueue q;
    q.push(makeArrivalEvent(0.0, 0));
    EXPECT_EQ(q.size(), 1UL);
    q.push(makeScheduleEvent(0.0));
    EXPECT_EQ(q.size(), 2UL);
}

TEST(EventQueueTest, PopOnEmptyThrows) {
    EventQueue q;
    EXPECT_THROW(q.pop(), std::runtime_error);
}

TEST(EventQueueTest, TopOnEmptyThrows) {
    EventQueue q;
    EXPECT_THROW((void)q.top(), std::runtime_error);
}

// ─── Timestamp ordering ───────────────────────────────────────────────────────

TEST(EventQueueTest, EarlierTimestampPopsFirst) {
    EventQueue q;
    q.push(makeArrivalEvent(10.0, 0));
    q.push(makeArrivalEvent(1.0,  1));
    q.push(makeArrivalEvent(5.0,  2));

    EXPECT_DOUBLE_EQ(q.pop().timestamp, 1.0);
    EXPECT_DOUBLE_EQ(q.pop().timestamp, 5.0);
    EXPECT_DOUBLE_EQ(q.pop().timestamp, 10.0);
}

// ─── Type ordering within same tick ──────────────────────────────────────────

TEST(EventQueueTest, ArrivalBeforeScheduleAtSameTick) {
    EventQueue q;
    q.push(makeScheduleEvent(0.0));   // type=1
    q.push(makeArrivalEvent(0.0, 0)); // type=0 ← should pop first

    Event first = q.pop();
    EXPECT_EQ(first.type, EventType::Arrival);
}

TEST(EventQueueTest, ScheduleBeforeServiceAtSameTick) {
    EventQueue q;
    q.push(makeServiceEvent(5.0, 1));  // type=2
    q.push(makeScheduleEvent(5.0));    // type=1 ← should pop first

    Event first = q.pop();
    EXPECT_EQ(first.type, EventType::Schedule);
}

TEST(EventQueueTest, ServiceBeforeMetricsAtSameTick) {
    EventQueue q;
    q.push(makeMetricsEvent(3.0));    // type=3
    q.push(makeServiceEvent(3.0, 0)); // type=2 ← should pop first

    Event first = q.pop();
    EXPECT_EQ(first.type, EventType::Service);
}

TEST(EventQueueTest, FullOrderingWithinOneTick) {
    // Insert in reverse order to stress the heap
    EventQueue q;
    q.push(makeMetricsEvent(0.0));    // 3
    q.push(makeServiceEvent(0.0, 0)); // 2
    q.push(makeScheduleEvent(0.0));   // 1
    q.push(makeArrivalEvent(0.0, 0)); // 0

    EXPECT_EQ(q.pop().type, EventType::Arrival);
    EXPECT_EQ(q.pop().type, EventType::Schedule);
    EXPECT_EQ(q.pop().type, EventType::Service);
    EXPECT_EQ(q.pop().type, EventType::Metrics);
}

// ─── Cross-tick and same-type ordering ────────────────────────────────────────

TEST(EventQueueTest, CrossTickOrderingCorrect) {
    EventQueue q;
    q.push(makeArrivalEvent(2.0, 0));
    q.push(makeMetricsEvent(0.0));
    q.push(makeScheduleEvent(1.0));

    EXPECT_EQ(q.pop().timestamp, 0.0);
    EXPECT_EQ(q.pop().timestamp, 1.0);
    EXPECT_EQ(q.pop().timestamp, 2.0);
}

// ─── Clear ────────────────────────────────────────────────────────────────────

TEST(EventQueueTest, ClearEmptiesQueue) {
    EventQueue q;
    q.push(makeArrivalEvent(1.0, 0));
    q.push(makeArrivalEvent(2.0, 1));
    q.push(makeArrivalEvent(3.0, 2));
    EXPECT_EQ(q.size(), 3UL);
    q.clear();
    EXPECT_TRUE(q.empty());
}

// ─── Large insertion correctness ──────────────────────────────────────────────

TEST(EventQueueTest, LargeInsertionMaintainsOrder) {
    EventQueue q(512);
    constexpr int N = 500;

    // Insert events at timestamps N, N-1, ..., 1
    for (int i = N; i >= 1; --i) {
        q.push(makeArrivalEvent(static_cast<double>(i), 0));
    }

    double prev = 0.0;
    while (!q.empty()) {
        Event e = q.pop();
        EXPECT_GE(e.timestamp, prev);
        prev = e.timestamp;
    }
}

// ─── Factory helpers ──────────────────────────────────────────────────────────

TEST(EventFactoryTest, ArrivalEventFields) {
    Event e = makeArrivalEvent(42.5, 7);
    EXPECT_DOUBLE_EQ(e.timestamp, 42.5);
    EXPECT_EQ(e.type, EventType::Arrival);
    EXPECT_EQ(e.pid, 7UL);
}

TEST(EventFactoryTest, ScheduleEventPIDIsZero) {
    Event e = makeScheduleEvent(10.0);
    EXPECT_EQ(e.type, EventType::Schedule);
    EXPECT_EQ(e.pid, 0UL);
}

TEST(EventFactoryTest, ServiceEventFields) {
    Event e = makeServiceEvent(99.0, 3);
    EXPECT_EQ(e.type, EventType::Service);
    EXPECT_EQ(e.pid, 3UL);
}

TEST(EventFactoryTest, MetricsEventPIDIsZero) {
    Event e = makeMetricsEvent(50.0);
    EXPECT_EQ(e.type, EventType::Metrics);
    EXPECT_EQ(e.pid, 0UL);
}

// ─── Comparison operators ─────────────────────────────────────────────────────

TEST(EventComparisonTest, EarlierTimestampIsLess) {
    Event a = makeArrivalEvent(1.0, 0);
    Event b = makeArrivalEvent(2.0, 0);
    EXPECT_TRUE(a < b);
    EXPECT_FALSE(b < a);
}

TEST(EventComparisonTest, SameTimestampLowerTypeLess) {
    Event a = makeArrivalEvent(5.0, 0);   // type=0
    Event b = makeScheduleEvent(5.0);     // type=1
    EXPECT_TRUE(a < b);
}

} // namespace embi
