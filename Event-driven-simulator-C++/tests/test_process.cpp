/**
 * @file test_process.cpp
 * @brief Unit tests for Process: arrivals, service, EWMA updates, starvation.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#include <gtest/gtest.h>
#include "core/Process.hpp"

#include <cmath>

namespace embi {


TEST(ProcessTest, ConstructionSetsIdentity) {
    Process p(7, 0.5, 1.0, 0.1, 0.1);
    EXPECT_EQ(p.id, 7UL);
}

TEST(ProcessTest, InitialQueueIsZero) {
    Process p(0, 0.5, 1.0, 0.1, 0.1);
    EXPECT_EQ(p.queue_length, 0LL);
}

TEST(ProcessTest, EWMAWarmStartedToTrueRates) {
    Process p(0, 0.4, 0.8, 0.1, 0.1);
    EXPECT_DOUBLE_EQ(p.lambda_hat, 0.4);
    EXPECT_DOUBLE_EQ(p.mu_hat,    0.8);
}

// ─── Arrivals ─────────────────────────────────────────────────────────────────

TEST(ProcessTest, ArrivalIncrementsQueueLength) {
    Process p(0, 0.5, 1.0, 0.1, 0.1);
    p.arrival(10.0);
    EXPECT_EQ(p.queue_length, 1LL);
    p.arrival(12.0);
    EXPECT_EQ(p.queue_length, 2LL);
}

TEST(ProcessTest, ArrivalIncrementsArrivalCount) {
    Process p(0, 0.5, 1.0, 0.1, 0.1);
    p.arrival(5.0);
    p.arrival(6.0);
    p.arrival(7.0);
    EXPECT_EQ(p.arrival_count, 3ULL);
}

TEST(ProcessTest, FirstArrivalSetsFirstArrivalTime) {
    Process p(0, 0.5, 1.0, 0.1, 0.1);
    EXPECT_LT(p.first_arrival_time, 0.0);  // -1 initially
    p.arrival(42.0);
    EXPECT_DOUBLE_EQ(p.first_arrival_time, 42.0);
}

TEST(ProcessTest, SubsequentArrivalsDoNotOverwriteFirstArrivalTime) {
    Process p(0, 0.5, 1.0, 0.1, 0.1);
    p.arrival(10.0);
    p.arrival(20.0);
    EXPECT_DOUBLE_EQ(p.first_arrival_time, 10.0);
}

// ─── Service ─────────────────────────────────────────────────────────────────

TEST(ProcessTest, ServiceDecrementsQueueLength) {
    Process p(0, 0.5, 1.0, 0.1, 0.1);
    p.arrival(5.0);
    p.arrival(6.0);
    EXPECT_EQ(p.queue_length, 2LL);
    p.service(10.0);
    EXPECT_EQ(p.queue_length, 1LL);
}

TEST(ProcessTest, ServiceIncrementsCompletedJobs) {
    Process p(0, 0.5, 1.0, 0.1, 0.1);
    p.arrival(0.0);
    p.service(5.0);
    EXPECT_EQ(p.completed_jobs, 1ULL);
}

TEST(ProcessTest, ServiceReturnsCorrectWaitingTime) {
    Process p(0, 0.5, 1.0, 0.1, 0.1);
    p.arrival(3.0);  // job arrives at tick 3
    double wt = p.service(10.0);  // served at tick 10 → wait = 7
    EXPECT_DOUBLE_EQ(wt, 7.0);
}

TEST(ProcessTest, ServiceOnEmptyQueueReturnsZero) {
    Process p(0, 0.5, 1.0, 0.1, 0.1);
    // No arrivals — service on empty queue should not crash, returns 0
    double wt = p.service(5.0);
    EXPECT_DOUBLE_EQ(wt, 0.0);
}

TEST(ProcessTest, FIFOOrderingPreserved) {
    // Two arrivals at different times; first served should be the first arrival
    Process p(0, 0.5, 1.0, 0.1, 0.1);
    p.arrival(2.0);   // job A
    p.arrival(5.0);   // job B
    double wt_a = p.service(10.0);  // serves job A → wait = 10 - 2 = 8
    double wt_b = p.service(12.0);  // serves job B → wait = 12 - 5 = 7
    EXPECT_DOUBLE_EQ(wt_a, 8.0);
    EXPECT_DOUBLE_EQ(wt_b, 7.0);
}

// ─── Computed properties ──────────────────────────────────────────────────────

TEST(ProcessTest, AverageWaitingTimeZeroIfNoCompleted) {
    Process p(0, 0.5, 1.0, 0.1, 0.1);
    EXPECT_DOUBLE_EQ(p.averageWaitingTime(), 0.0);
}

TEST(ProcessTest, AverageWaitingTimeCorrect) {
    Process p(0, 0.5, 1.0, 0.1, 0.1);
    p.arrival(0.0);
    p.arrival(0.0);
    p.service(4.0);  // wait = 4
    p.service(6.0);  // wait = 6
    EXPECT_DOUBLE_EQ(p.averageWaitingTime(), 5.0);  // (4 + 6) / 2
}

TEST(ProcessTest, CPUShareCorrect) {
    Process p(0, 0.5, 1.0, 0.1, 0.1);
    p.arrival(0.0);
    p.arrival(0.0);
    p.service(1.0);
    p.service(2.0);
    // 2 completed over 10 ticks → share = 0.2
    EXPECT_DOUBLE_EQ(p.cpuShare(10), 0.2);
}

// ─── Starvation ───────────────────────────────────────────────────────────────

TEST(ProcessTest, TickIdleIncrementsStarvationCounter) {
    Process p(0, 0.5, 1.0, 0.1, 0.1);
    p.arrival(0.0);  // non-empty queue
    p.tickIdle(100);
    p.tickIdle(100);
    EXPECT_EQ(p.ticks_since_last_service, 2ULL);
    EXPECT_EQ(p.max_starvation_ticks, 2ULL);
}

TEST(ProcessTest, ServiceResetsStarvationCounter) {
    Process p(0, 0.5, 1.0, 0.1, 0.1);
    p.arrival(0.0);
    p.tickIdle(100);
    p.tickIdle(100);
    p.service(5.0);
    EXPECT_EQ(p.ticks_since_last_service, 0ULL);
}

TEST(ProcessTest, StarvationEventCountedAtThreshold) {
    Process p(0, 0.5, 1.0, 0.1, 0.1);
    p.arrival(0.0);  // non-empty queue
    for (int i = 0; i < 100; ++i) {
        p.tickIdle(100);  // threshold = 100
    }
    EXPECT_EQ(p.starvation_events, 1ULL);
}

// ─── Reset ────────────────────────────────────────────────────────────────────

TEST(ProcessTest, ResetClearsAllStatistics) {
    Process p(0, 0.5, 1.0, 0.1, 0.1);
    p.arrival(1.0);
    p.arrival(2.0);
    p.service(5.0);
    p.tickIdle(100);
    p.reset();

    EXPECT_EQ(p.queue_length,             0LL);
    EXPECT_EQ(p.arrival_count,            0ULL);
    EXPECT_EQ(p.completed_jobs,           0ULL);
    EXPECT_DOUBLE_EQ(p.total_waiting_time, 0.0);
    EXPECT_EQ(p.ticks_since_last_service, 0ULL);
    EXPECT_LT(p.first_arrival_time,       0.0);
}

TEST(ProcessTest, ResetWarmStartsEWMA) {
    Process p(0, 0.5, 0.8, 0.1, 0.1);
    p.arrival(1.0);
    p.arrival(2.0);
    p.service(5.0);
    // EWMA has been updated
    p.reset();
    // After reset, EWMA should be back to true rates
    EXPECT_DOUBLE_EQ(p.lambda_hat, 0.5);
    EXPECT_DOUBLE_EQ(p.mu_hat,    0.8);
}

// ─── EWMA updates ─────────────────────────────────────────────────────────────

TEST(ProcessTest, ArrivalUpdatesEWMAAfterFirstArrival) {
    Process p(0, 0.5, 1.0, 0.1, 0.1);
    double initial_lambda = p.lambda_hat;
    p.arrival(0.0);   // first arrival: no inter-arrival update
    p.arrival(1.0);   // second arrival: inter-arrival = 1 → rate = 1.0
    // lambda_hat should have moved toward 0.5
    EXPECT_NE(p.lambda_hat, initial_lambda);  // must have changed
}

TEST(ProcessTest, UpdateArrivalEstimateMonotonic) {
    Process p(0, 0.01, 1.0, 0.5, 0.1);
    // High arrival rate observation should push lambda_hat up
    p.updateArrivalEstimate(0.1);  // 1/0.1 = 10.0 rate
    EXPECT_GT(p.lambda_hat, 0.01);
}

} // namespace embi
