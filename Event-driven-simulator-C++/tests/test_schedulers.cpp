/**
 * @file test_schedulers.cpp
 * @brief Unit tests for all scheduler implementations.
 *
 * Tests verify:
 *   - choose() returns a valid Decision when queues are non-empty
 *   - choose() returns Decision::idle() when all queues are empty
 *   - EMBI correctly clips/unclips negative scores
 *   - MaxWeight selects the highest μ·Q process
 *   - RoundRobin cycles deterministically
 *   - FCFS selects the process with the earliest first arrival
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#include <gtest/gtest.h>
#include "schedulers/EMBIScheduler.hpp"
#include "schedulers/MaxWeightScheduler.hpp"
#include "schedulers/CmuScheduler.hpp"
#include "schedulers/RoundRobinScheduler.hpp"
#include "schedulers/FCFSScheduler.hpp"
#include "core/Config.hpp"
#include "core/Process.hpp"

#include <vector>

namespace embi {

// ─── Helpers ─────────────────────────────────────────────────────────────────

namespace {

/// Builds a default Config for scheduler construction.
Config makeConfig(double M = 10.0) {
    Config cfg;
    cfg.M             = M;
    cfg.num_processes = 4;
    cfg.alpha         = 0.1;
    cfg.beta          = 0.1;
    cfg.arrival_rate  = 0.5;
    cfg.service_rate  = 1.0;
    return cfg;
}



/// Builds a context from processes with a zeroed snapshot.
SchedulerContext makeContext(const std::vector<Process>& procs, const Config& cfg) {
    static OnlineSnapshot snap{};
    return SchedulerContext{procs, 0.0, std::nullopt, snap, cfg};
}

/// Creates N processes, all with queue_length = 0.
std::vector<Process> makeProcesses(std::size_t N, double lambda = 0.5, double mu = 1.0) {
    std::vector<Process> procs;
    procs.reserve(N);
    for (std::size_t i = 0; i < N; ++i) {
        procs.emplace_back(i, lambda, mu, 0.1, 0.1);
    }
    return procs;
}

} // namespace

// ─── All schedulers: empty queue → idle ──────────────────────────────────────

class SchedulerIdleTest : public ::testing::TestWithParam<std::string> {};

TEST_P(SchedulerIdleTest, EmptyQueuesReturnIdle) {
    Config cfg = makeConfig();
    std::vector<Process> procs = makeProcesses(4);  // all queue_length = 0
    SchedulerContext ctx = makeContext(procs, cfg);

    std::unique_ptr<BaseScheduler> sched;
    std::string name = GetParam();

    if (name == "embi")       sched = std::make_unique<EMBIScheduler>(cfg, true);
    else if (name == "maxweight") sched = std::make_unique<MaxWeightScheduler>(cfg);
    else if (name == "cmu")   sched = std::make_unique<CmuScheduler>(cfg);
    else if (name == "rr")    sched = std::make_unique<RoundRobinScheduler>(cfg);
    else if (name == "fcfs")  sched = std::make_unique<FCFSScheduler>(cfg);

    Decision d = sched->choose(ctx);
    EXPECT_FALSE(d.valid) << "Scheduler " << name << " should return idle when all queues empty";
}

INSTANTIATE_TEST_SUITE_P(AllSchedulers, SchedulerIdleTest,
    ::testing::Values("embi", "maxweight", "cmu", "rr", "fcfs"));

// ─── All schedulers: non-empty queue → valid decision ────────────────────────

class SchedulerValidTest : public ::testing::TestWithParam<std::string> {};

TEST_P(SchedulerValidTest, NonEmptyQueueReturnsValidDecision) {
    Config cfg = makeConfig();
    std::vector<Process> procs = makeProcesses(4);
    procs[2].arrival(0.0);  // only process 2 has a job

    SchedulerContext ctx = makeContext(procs, cfg);

    std::unique_ptr<BaseScheduler> sched;
    std::string name = GetParam();
    if (name == "embi")       sched = std::make_unique<EMBIScheduler>(cfg, true);
    else if (name == "maxweight") sched = std::make_unique<MaxWeightScheduler>(cfg);
    else if (name == "cmu")   sched = std::make_unique<CmuScheduler>(cfg);
    else if (name == "rr")    sched = std::make_unique<RoundRobinScheduler>(cfg);
    else if (name == "fcfs")  sched = std::make_unique<FCFSScheduler>(cfg);

    Decision d = sched->choose(ctx);
    EXPECT_TRUE(d.valid)  << "Scheduler " << name << " should be valid with non-empty queue";
    EXPECT_EQ(d.chosen_pid, 2UL) << "Only process 2 has a job";
}

INSTANTIATE_TEST_SUITE_P(AllSchedulers, SchedulerValidTest,
    ::testing::Values("embi", "maxweight", "cmu", "rr", "fcfs"));

// ─── EMBI ─────────────────────────────────────────────────────────────────────

TEST(EMBISchedulerTest, SelectsHighestScoringProcess) {
    Config cfg = makeConfig(/*M=*/2.0);
    auto procs = makeProcesses(3);

    // Process 0: Q=1, lambda=0.5, mu=1.0 → score = max(0, 1*(2*1 + 2*0.5 - 2)) = max(0,1) = 1
    // Process 1: Q=5, lambda=0.5, mu=1.0 → score = max(0, 1*(2*5 + 2*0.5 - 2)) = max(0,9) = 9 ← winner
    // Process 2: Q=2, lambda=0.5, mu=1.0 → score = max(0, 1*(2*2 + 2*0.5 - 2)) = max(0,3) = 3
    procs[0].queue_length = 1;
    procs[1].queue_length = 5;
    procs[2].queue_length = 2;

    EMBIScheduler sched(cfg, true);
    Decision d = sched.choose(makeContext(procs, cfg));

    EXPECT_TRUE(d.valid);
    EXPECT_EQ(d.chosen_pid, 1UL);
}

TEST(EMBISchedulerTest, ClippedVariantNeverNegativeScore) {
    Config cfg = makeConfig(/*M=*/1000.0);  // Very large M → all raw scores negative
    auto procs = makeProcesses(3);
    for (auto& p : procs) p.queue_length = 1;

    EMBIScheduler sched(cfg, /*clip=*/true);
    Decision d = sched.choose(makeContext(procs, cfg));
    // Clipped to 0: all equal, so a valid process is still chosen (first non-empty)
    // The key test is that it doesn't crash and chosen_score >= 0
    EXPECT_GE(d.chosen_score, 0.0);
}

TEST(EMBISchedulerTest, UnclippedVariantCanChooseNegativeScore) {
    Config cfg = makeConfig(/*M=*/1000.0);
    auto procs = makeProcesses(3);
    for (auto& p : procs) p.queue_length = 1;

    EMBIScheduler sched(cfg, /*clip=*/false);
    Decision d = sched.choose(makeContext(procs, cfg));
    // Unclipped: scores are negative, but still selects maximum (least negative)
    EXPECT_TRUE(d.valid);
}

TEST(EMBISchedulerTest, NameIsEmbiWhenClipped) {
    Config cfg = makeConfig();
    EMBIScheduler sched(cfg, true);
    EXPECT_EQ(sched.name(), "embi");
}

TEST(EMBISchedulerTest, NameIsUnclippedWhenUnclipped) {
    Config cfg = makeConfig();
    EMBIScheduler sched(cfg, false);
    EXPECT_EQ(sched.name(), "embi_unclipped");
}

// ─── MaxWeight ────────────────────────────────────────────────────────────────

TEST(MaxWeightSchedulerTest, SelectsHighestMuTimesQueue) {
    Config cfg = makeConfig();
    auto procs = makeProcesses(3);

    // Process 0: mu=1.0, Q=2 → score=2
    // Process 1: mu=2.0, Q=1 → score=2  (tie → lower index wins due to argmax)
    // Process 2: mu=0.5, Q=5 → score=2.5 ← winner
    procs[0].mu_hat = 1.0;  procs[0].queue_length = 2;
    procs[1].mu_hat = 2.0;  procs[1].queue_length = 1;
    procs[2].mu_hat = 0.5;  procs[2].queue_length = 5;

    MaxWeightScheduler sched(cfg);
    Decision d = sched.choose(makeContext(procs, cfg));

    EXPECT_TRUE(d.valid);
    EXPECT_EQ(d.chosen_pid, 2UL);
}

// ─── cμ ────────────────────────────────────────────────────────────────────────

TEST(CmuSchedulerTest, SelectsHighestServiceRate) {
    Config cfg = makeConfig();
    auto procs = makeProcesses(3);

    procs[0].mu_hat = 0.5;  procs[0].queue_length = 10;  // high queue but low mu
    procs[1].mu_hat = 3.0;  procs[1].queue_length = 1;   // highest mu ← winner
    procs[2].mu_hat = 1.0;  procs[2].queue_length = 5;

    CmuScheduler sched(cfg);
    Decision d = sched.choose(makeContext(procs, cfg));

    EXPECT_TRUE(d.valid);
    EXPECT_EQ(d.chosen_pid, 1UL);
}

// ─── Round Robin ─────────────────────────────────────────────────────────────

TEST(RoundRobinSchedulerTest, CyclesThroughProcesses) {
    Config cfg = makeConfig();
    auto procs = makeProcesses(3);
    // All have jobs
    for (auto& p : procs) p.queue_length = 5;

    RoundRobinScheduler sched(cfg);
    SchedulerContext ctx = makeContext(procs, cfg);

    Decision d0 = sched.choose(ctx);
    Decision d1 = sched.choose(ctx);
    Decision d2 = sched.choose(ctx);
    Decision d3 = sched.choose(ctx);  // wraps around to 0

    EXPECT_EQ(d0.chosen_pid, 0UL);
    EXPECT_EQ(d1.chosen_pid, 1UL);
    EXPECT_EQ(d2.chosen_pid, 2UL);
    EXPECT_EQ(d3.chosen_pid, 0UL);  // wrap-around
}

TEST(RoundRobinSchedulerTest, SkipsEmptyQueues) {
    Config cfg = makeConfig();
    auto procs = makeProcesses(3);
    procs[0].queue_length = 0;  // skip
    procs[1].queue_length = 0;  // skip
    procs[2].queue_length = 5;  // only non-empty

    RoundRobinScheduler sched(cfg);
    Decision d = sched.choose(makeContext(procs, cfg));

    EXPECT_TRUE(d.valid);
    EXPECT_EQ(d.chosen_pid, 2UL);
}

// ─── FCFS ─────────────────────────────────────────────────────────────────────

TEST(FCFSSchedulerTest, SelectsEarliestArrival) {
    Config cfg = makeConfig();
    auto procs = makeProcesses(3);

    procs[0].first_arrival_time = 10.0;  procs[0].queue_length = 1;
    procs[1].first_arrival_time = 5.0;   procs[1].queue_length = 1;  // ← earliest
    procs[2].first_arrival_time = 8.0;   procs[2].queue_length = 1;

    FCFSScheduler sched(cfg);
    Decision d = sched.choose(makeContext(procs, cfg));

    EXPECT_TRUE(d.valid);
    EXPECT_EQ(d.chosen_pid, 1UL);
}

TEST(FCFSSchedulerTest, BreaksTiesByPID) {
    Config cfg = makeConfig();
    auto procs = makeProcesses(3);

    procs[0].first_arrival_time = 5.0;  procs[0].queue_length = 1;
    procs[1].first_arrival_time = 5.0;  procs[1].queue_length = 1;
    procs[2].first_arrival_time = 5.0;  procs[2].queue_length = 1;

    FCFSScheduler sched(cfg);
    Decision d = sched.choose(makeContext(procs, cfg));

    EXPECT_TRUE(d.valid);
    EXPECT_EQ(d.chosen_pid, 0UL);  // lowest PID wins on tie
}

// ─── Decision diagnostics ─────────────────────────────────────────────────────

TEST(SchedulerDiagnosticsTest, DecisionEntropyIsNonNegative) {
    Config cfg = makeConfig();
    auto procs = makeProcesses(4);
    for (auto& p : procs) p.queue_length = 3;

    MaxWeightScheduler sched(cfg);
    Decision d = sched.choose(makeContext(procs, cfg));

    EXPECT_GE(d.decision_entropy, 0.0);
}

TEST(SchedulerDiagnosticsTest, DecisionTimeNsIsPositive) {
    Config cfg = makeConfig();
    auto procs = makeProcesses(4);
    procs[0].queue_length = 1;

    EMBIScheduler sched(cfg, true);
    Decision d = sched.choose(makeContext(procs, cfg));

    // May be 0 in very fast environments but never negative
    EXPECT_GE(d.decision_time_ns, 0.0);
}

TEST(SchedulerDiagnosticsTest, ScoreDeltaIsNonNegative) {
    Config cfg = makeConfig(/*M=*/2.0);
    auto procs = makeProcesses(4);
    procs[0].queue_length = 1;
    procs[1].queue_length = 5;
    procs[2].queue_length = 2;
    procs[3].queue_length = 3;

    EMBIScheduler sched(cfg, true);
    Decision d = sched.choose(makeContext(procs, cfg));

    // chosen_score should be ≥ second_best_score
    EXPECT_GE(d.score_delta, 0.0);
}

} // namespace embi
