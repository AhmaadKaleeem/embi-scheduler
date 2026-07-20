/**
 * @file test_metrics_online.cpp
 * @brief Unit tests for OnlineMetrics: Lyapunov, throughput, utilization.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#include <gtest/gtest.h>
#include "core/OnlineMetrics.hpp"
#include "core/Process.hpp"
#include "schedulers/Decision.hpp"

#include <vector>

namespace embi {

namespace {

std::vector<Process> makeProcesses(std::size_t N, int64_t queue_len = 0) {
    std::vector<Process> procs;
    procs.reserve(N);
    for (std::size_t i = 0; i < N; ++i) {
        procs.emplace_back(i, 0.5, 1.0, 0.1, 0.1);
        procs.back().queue_length = queue_len;
    }
    return procs;
}

Decision validDecision(std::size_t pid = 0) {
    Decision d;
    d.valid         = true;
    d.chosen_pid    = pid;
    d.decision_time_ns = 100.0;
    return d;
}

Decision idleDecision() { return Decision::idle(); }

} // namespace

// ─── Construction ────────────────────────────────────────────────────────────

TEST(OnlineMetricsTest, InitialStateIsZero) {
    OnlineMetrics om(4, 100);
    EXPECT_DOUBLE_EQ(om.lyapunovV(), 0.0);
    EXPECT_DOUBLE_EQ(om.lyapunovDrift(), 0.0);
    EXPECT_DOUBLE_EQ(om.rollingThroughput(), 0.0);
    EXPECT_DOUBLE_EQ(om.utilization(), 0.0);
    EXPECT_EQ(om.completedJobs(), 0ULL);
}

// ─── Lyapunov ────────────────────────────────────────────────────────────────

TEST(OnlineMetricsTest, LyapunovVIsCorrect) {
    OnlineMetrics om(3, 100);
    auto procs = makeProcesses(3);
    procs[0].queue_length = 2;
    procs[1].queue_length = 3;
    procs[2].queue_length = 1;
    // V = 2² + 3² + 1² = 4 + 9 + 1 = 14

    om.update(procs, validDecision(0), 0);
    EXPECT_DOUBLE_EQ(om.lyapunovV(), 14.0);
}

TEST(OnlineMetricsTest, LyapunovDriftIsVtMinusPrevV) {
    OnlineMetrics om(2, 100);
    auto procs = makeProcesses(2);
    procs[0].queue_length = 2;
    procs[1].queue_length = 2;
    // V(0) = 8, prev_v = 0 → drift = 8

    om.update(procs, validDecision(0), 0);
    EXPECT_DOUBLE_EQ(om.lyapunovDrift(), 8.0);

    procs[0].queue_length = 1;
    procs[1].queue_length = 1;
    // V(1) = 2, prev_v = 8 → drift = -6
    om.update(procs, validDecision(0), 1);
    EXPECT_DOUBLE_EQ(om.lyapunovDrift(), -6.0);
}

// ─── Utilization ─────────────────────────────────────────────────────────────

TEST(OnlineMetricsTest, UtilizationIsOneWhenAlwaysBusy) {
    OnlineMetrics om(2, 10);
    auto procs = makeProcesses(2);

    for (uint64_t t = 0; t < 100; ++t) {
        om.update(procs, validDecision(0), t);
    }
    EXPECT_DOUBLE_EQ(om.utilization(), 1.0);
}

TEST(OnlineMetricsTest, UtilizationIsZeroWhenAlwaysIdle) {
    OnlineMetrics om(2, 10);
    auto procs = makeProcesses(2);

    for (uint64_t t = 0; t < 100; ++t) {
        om.update(procs, idleDecision(), t);
    }
    EXPECT_DOUBLE_EQ(om.utilization(), 0.0);
}

TEST(OnlineMetricsTest, UtilizationIsHalfWhenAlternating) {
    OnlineMetrics om(2, 100);
    auto procs = makeProcesses(2);

    for (uint64_t t = 0; t < 100; ++t) {
        if (t % 2 == 0) {
            om.update(procs, validDecision(0), t);
        } else {
            om.update(procs, idleDecision(), t);
        }
    }
    EXPECT_NEAR(om.utilization(), 0.5, 0.01);
}

// ─── Throughput ───────────────────────────────────────────────────────────────

TEST(OnlineMetricsTest, RollingThroughputNonNegative) {
    OnlineMetrics om(2, 50);
    auto procs = makeProcesses(2);

    for (uint64_t t = 0; t < 200; ++t) {
        om.update(procs, validDecision(0), t);
    }
    EXPECT_GE(om.rollingThroughput(), 0.0);
    EXPECT_LE(om.rollingThroughput(), 1.0);
}

// ─── Completed jobs ───────────────────────────────────────────────────────────

TEST(OnlineMetricsTest, CompletedJobsCountsValidDecisions) {
    OnlineMetrics om(2, 100);
    auto procs = makeProcesses(2);

    om.update(procs, validDecision(0), 0);
    om.update(procs, idleDecision(), 1);
    om.update(procs, validDecision(1), 2);

    EXPECT_EQ(om.completedJobs(), 2ULL);
}

// ─── Decision overhead ────────────────────────────────────────────────────────

TEST(OnlineMetricsTest, TotalDecisionTimeAccumulates) {
    OnlineMetrics om(2, 100);
    auto procs = makeProcesses(2);

    Decision d = validDecision(0);
    d.decision_time_ns = 250.0;

    om.update(procs, d, 0);
    om.update(procs, d, 1);

    EXPECT_DOUBLE_EQ(om.totalDecisionTimeNs(), 500.0);
}

TEST(OnlineMetricsTest, MeanDecisionTimeIsAverage) {
    OnlineMetrics om(2, 100);
    auto procs = makeProcesses(2);

    Decision d = validDecision(0);
    d.decision_time_ns = 100.0;

    om.update(procs, d, 0);
    om.update(procs, d, 1);
    om.update(procs, d, 2);

    EXPECT_NEAR(om.meanDecisionTimeNs(), 100.0, 1e-9);
}

// ─── Snapshot ────────────────────────────────────────────────────────────────

TEST(OnlineMetricsTest, SnapshotReflectsCurrentState) {
    OnlineMetrics om(2, 100);
    auto procs = makeProcesses(2);
    procs[0].queue_length = 3;
    procs[1].queue_length = 3;

    om.update(procs, validDecision(0), 5);
    auto snap = om.snapshot();

    EXPECT_DOUBLE_EQ(snap.lyapunov_v, om.lyapunovV());
    EXPECT_DOUBLE_EQ(snap.utilization, om.utilization());
    EXPECT_EQ(snap.tick, om.currentTick());
}

// ─── Reset ───────────────────────────────────────────────────────────────────

TEST(OnlineMetricsTest, ResetClearsAllAccumulators) {
    OnlineMetrics om(2, 100);
    auto procs = makeProcesses(2, 5);

    for (uint64_t t = 0; t < 50; ++t) {
        om.update(procs, validDecision(0), t);
    }

    om.reset();

    EXPECT_DOUBLE_EQ(om.lyapunovV(), 0.0);
    EXPECT_EQ(om.completedJobs(), 0ULL);
    EXPECT_DOUBLE_EQ(om.utilization(), 0.0);
    EXPECT_EQ(om.currentTick(), 0ULL);
}

} // namespace embi
