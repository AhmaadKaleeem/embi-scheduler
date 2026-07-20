/**
 * @file test_metrics_offline.cpp
 * @brief Unit tests for OfflineMetrics: waiting time histograms, Jain fairness.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#include <gtest/gtest.h>
#include "core/OfflineMetrics.hpp"
#include "core/Process.hpp"
#include "schedulers/Decision.hpp"

#include <vector>
#include <cmath>

namespace embi {

namespace {

std::vector<Process> makeProcesses(std::size_t N) {
    std::vector<Process> procs;
    procs.reserve(N);
    for (std::size_t i = 0; i < N; ++i) {
        procs.emplace_back(i, 0.5, 1.0, 0.1, 0.1);
    }
    return procs;
}

Decision validDecision(std::size_t pid = 0) {
    Decision d;
    d.valid = true;
    d.chosen_pid = pid;
    d.decision_entropy = 1.5;
    d.score_variance = 0.3;
    return d;
}

} // namespace

// ─── Waiting time recording ───────────────────────────────────────────────────

TEST(OfflineMetricsTest, WaitingTimeZeroIfNoRecords) {
    OfflineMetrics om(4, 1000);
    auto procs = makeProcesses(4);
    auto report = om.compute(procs, 1000);
    EXPECT_DOUBLE_EQ(report.avg_waiting_time, 0.0);
}

TEST(OfflineMetricsTest, AvgWaitingTimeCorrect) {
    OfflineMetrics om(4, 1000);
    om.recordWaitingTime(4.0);
    om.recordWaitingTime(6.0);
    om.recordWaitingTime(8.0);

    auto procs = makeProcesses(4);
    auto report = om.compute(procs, 1000);
    EXPECT_NEAR(report.avg_waiting_time, 6.0, 1e-9);
}

TEST(OfflineMetricsTest, MaxWaitingTimeCorrect) {
    OfflineMetrics om(4, 1000);
    om.recordWaitingTime(2.0);
    om.recordWaitingTime(9.0);
    om.recordWaitingTime(5.0);

    auto procs = makeProcesses(4);
    auto report = om.compute(procs, 1000);
    EXPECT_DOUBLE_EQ(report.max_waiting_time, 9.0);
}

TEST(OfflineMetricsTest, PercentilesInOrder) {
    OfflineMetrics om(2, 10000);
    // Record 1000 samples uniformly [1, 1000]
    for (int i = 1; i <= 1000; ++i) {
        om.recordWaitingTime(static_cast<double>(i));
    }

    auto procs = makeProcesses(2);
    auto report = om.compute(procs, 10000);

    EXPECT_LE(report.p50_waiting_time, report.p95_waiting_time);
    EXPECT_LE(report.p95_waiting_time, report.p99_waiting_time);
}

// ─── Jain Fairness Index ──────────────────────────────────────────────────────

TEST(OfflineMetricsTest, JainFairnessIsOneForEqualShares) {
    OfflineMetrics om(3, 1000);
    auto procs = makeProcesses(3);

    // Give each process exactly 100 completed jobs
    for (auto& p : procs) {
        p.completed_jobs = 100;
    }

    auto report = om.compute(procs, 300);
    // Jain = (3*100)² / (3 * 3 * 100²) = 1.0
    EXPECT_NEAR(report.jain_fairness_index, 1.0, 1e-9);
}

TEST(OfflineMetricsTest, JainFairnessIsLowForUnequalShares) {
    OfflineMetrics om(3, 1000);
    auto procs = makeProcesses(3);

    procs[0].completed_jobs = 900;
    procs[1].completed_jobs = 50;
    procs[2].completed_jobs = 50;

    auto report = om.compute(procs, 1000);
    // Very unequal → Jain < 1
    EXPECT_LT(report.jain_fairness_index, 1.0);
    EXPECT_GT(report.jain_fairness_index, 0.0);
}

// ─── Throughput ───────────────────────────────────────────────────────────────

TEST(OfflineMetricsTest, TotalThroughputIsCompletedOverTicks) {
    OfflineMetrics om(2, 1000);
    auto procs = makeProcesses(2);
    procs[0].completed_jobs = 300;
    procs[1].completed_jobs = 200;

    auto report = om.compute(procs, 1000);
    EXPECT_NEAR(report.total_throughput, 0.5, 1e-9);  // 500 / 1000
}

// ─── Decision diagnostics ─────────────────────────────────────────────────────

TEST(OfflineMetricsTest, AvgEntropyIsPositiveAfterRecords) {
    OfflineMetrics om(2, 1000);
    auto procs = makeProcesses(2);

    for (int i = 0; i < 100; ++i) {
        om.recordDecision(validDecision(0), static_cast<uint64_t>(i));
    }

    auto report = om.compute(procs, 1000);
    EXPECT_GT(report.avg_decision_entropy, 0.0);
}

TEST(OfflineMetricsTest, ContextSwitchRateIsZeroWhenSameProcessAlwaysChosen) {
    OfflineMetrics om(2, 1000);
    auto procs = makeProcesses(2);

    for (int i = 0; i < 100; ++i) {
        om.recordDecision(validDecision(0), static_cast<uint64_t>(i));  // always pid=0
    }

    auto report = om.compute(procs, 1000);
    EXPECT_DOUBLE_EQ(report.context_switch_rate, 0.0);
}

TEST(OfflineMetricsTest, ContextSwitchRateIsHighWhenAlternating) {
    OfflineMetrics om(2, 1000);
    auto procs = makeProcesses(2);

    for (int i = 0; i < 100; ++i) {
        om.recordDecision(validDecision(i % 2), static_cast<uint64_t>(i));
    }

    auto report = om.compute(procs, 1000);
    EXPECT_GT(report.context_switch_rate, 0.0);
}

// ─── Starvation ───────────────────────────────────────────────────────────────

TEST(OfflineMetricsTest, StarvationReflectsProcessData) {
    OfflineMetrics om(3, 1000);
    auto procs = makeProcesses(3);
    procs[0].max_starvation_ticks = 100;
    procs[1].max_starvation_ticks = 200;
    procs[2].max_starvation_ticks = 50;

    auto report = om.compute(procs, 1000);
    EXPECT_DOUBLE_EQ(report.max_starvation_ticks, 200.0);
    EXPECT_NEAR(report.avg_starvation_ticks, (100.0 + 200.0 + 50.0) / 3.0, 1e-9);
}

// ─── Reset ────────────────────────────────────────────────────────────────────

TEST(OfflineMetricsTest, ResetClearsWaitingTimeHistory) {
    OfflineMetrics om(2, 1000);
    om.recordWaitingTime(50.0);
    om.recordWaitingTime(100.0);
    om.reset();

    auto procs = makeProcesses(2);
    auto report = om.compute(procs, 1000);
    EXPECT_DOUBLE_EQ(report.avg_waiting_time, 0.0);
}

} // namespace embi
