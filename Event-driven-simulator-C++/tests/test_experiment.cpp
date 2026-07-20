/**
 * @file test_experiment.cpp
 * @brief Integration tests for Experiment parameter sweeps.
 *
 * Uses null-log and minimal tick counts to run fast end-to-end.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#include <gtest/gtest.h>
#include "core/Experiment.hpp"
#include "core/Simulator.hpp"

namespace embi {

namespace {
ExperimentConfig makeMinimalExperiment() {
    ExperimentConfig exp;
    exp.schedulers    = {"embi", "rr"};
    exp.workloads     = {"poisson"};
    exp.seeds         = {42};
    exp.arrival_rates = {0.5};
    exp.ticks         = 500;
    exp.num_processes = 4;
    exp.M             = 10.0;
    exp.service_rate  = 1.0;
    exp.alpha         = 0.1;
    exp.beta          = 0.1;
    exp.null_log      = true;
    exp.output_dir    = "test_output_exp";
    return exp;
}
} // namespace

// ─── Basic runs ───────────────────────────────────────────────────────────────

TEST(ExperimentTest, TotalRunsIsCorrect) {
    auto exp_cfg = makeMinimalExperiment();
    Experiment experiment(exp_cfg);
    // 2 schedulers × 1 workload × 1 seed × 1 rate = 2
    EXPECT_EQ(experiment.totalRuns(), 2UL);
}

TEST(ExperimentTest, RunProducesCorrectNumberOfResults) {
    auto exp_cfg = makeMinimalExperiment();
    Experiment experiment(exp_cfg);
    experiment.run();  // no progress callback
    EXPECT_EQ(experiment.results().size(), 2UL);
}

TEST(ExperimentTest, AllResultsAreValid) {
    auto exp_cfg = makeMinimalExperiment();
    Experiment experiment(exp_cfg);
    experiment.run();

    for (const auto& r : experiment.results()) {
        EXPECT_TRUE(r.valid());
        EXPECT_FALSE(r.scheduler_name.empty());
        EXPECT_EQ(r.seed, 42ULL);
        EXPECT_DOUBLE_EQ(r.arrival_rate, 0.5);
    }
}

TEST(ExperimentTest, ProgressCallbackInvoked) {
    auto exp_cfg = makeMinimalExperiment();
    Experiment experiment(exp_cfg);

    std::size_t callback_count = 0;
    experiment.run([&](std::size_t completed, std::size_t total, const Results&) {
        callback_count++;
        EXPECT_LE(completed, total);
    });

    EXPECT_EQ(callback_count, experiment.totalRuns());
}

TEST(ExperimentTest, ResultsHaveDistinctSchedulers) {
    auto exp_cfg = makeMinimalExperiment();
    Experiment experiment(exp_cfg);
    experiment.run();

    const auto& results = experiment.results();
    ASSERT_EQ(results.size(), 2UL);
    EXPECT_NE(results[0].scheduler_name, results[1].scheduler_name);
}

// ─── Multi-seed sweep ─────────────────────────────────────────────────────────

TEST(ExperimentTest, MultiSeedProducesIndependentResults) {
    ExperimentConfig exp;
    exp.schedulers    = {"embi"};
    exp.workloads     = {"poisson"};
    exp.seeds         = {42, 123, 456};
    exp.arrival_rates = {0.5};
    exp.ticks         = 1000;
    exp.num_processes = 4;
    exp.M             = 10.0;
    exp.service_rate  = 1.0;
    exp.alpha         = 0.1;
    exp.beta          = 0.1;
    exp.null_log      = true;
    exp.output_dir    = "test_output_exp2";

    Experiment experiment(exp);
    experiment.run();

    EXPECT_EQ(experiment.results().size(), 3UL);
    // Seeds must be distinct across results
    EXPECT_NE(experiment.results()[0].seed, experiment.results()[1].seed);
    EXPECT_NE(experiment.results()[1].seed, experiment.results()[2].seed);
}

// ─── Simulator single run ─────────────────────────────────────────────────────

TEST(SimulatorTest, SingleRunCompletesSuccessfully) {
    Config cfg;
    cfg.scheduler_name = "embi";
    cfg.workload_name  = "poisson";
    cfg.ticks          = 1000;
    cfg.num_processes  = 4;
    cfg.seed           = 42;
    cfg.M              = 10.0;
    cfg.arrival_rate   = 0.5;
    cfg.service_rate   = 1.0;
    cfg.alpha          = 0.1;
    cfg.beta           = 0.1;
    cfg.null_log       = true;

    Simulator sim(cfg);
    ASSERT_NO_THROW({
        Results r = sim.run();
        EXPECT_TRUE(r.valid());
        EXPECT_EQ(r.scheduler_name, "embi");
    });
}

TEST(SimulatorTest, AllSchedulersSingleRun) {
    const char* schedulers[] = {"embi", "embi_unclipped", "maxweight", "cmu", "rr", "fcfs"};

    for (const char* sched : schedulers) {
        Config cfg;
        cfg.scheduler_name = sched;
        cfg.workload_name  = "poisson";
        cfg.ticks          = 500;
        cfg.num_processes  = 4;
        cfg.seed           = 42;
        cfg.M              = 10.0;
        cfg.arrival_rate   = 0.5;
        cfg.service_rate   = 1.0;
        cfg.alpha          = 0.1;
        cfg.beta           = 0.1;
        cfg.null_log       = true;

        ASSERT_NO_THROW({
            Simulator sim(cfg);
            Results r = sim.run();
            EXPECT_TRUE(r.valid()) << "Scheduler: " << sched;
        }) << "Scheduler: " << sched;
    }
}

TEST(SimulatorTest, AllWorkloadsSingleRun) {
    const char* workloads[] = {"uniform", "poisson", "bursty", "heavy_tail"};

    for (const char* wload : workloads) {
        Config cfg;
        cfg.scheduler_name  = "embi";
        cfg.workload_name   = wload;
        cfg.ticks           = 500;
        cfg.num_processes   = 4;
        cfg.seed            = 42;
        cfg.M               = 10.0;
        cfg.arrival_rate    = 0.5;
        cfg.service_rate    = 1.0;
        cfg.alpha           = 0.1;
        cfg.beta            = 0.1;
        cfg.null_log        = true;

        ASSERT_NO_THROW({
            Simulator sim(cfg);
            Results r = sim.run();
            EXPECT_TRUE(r.valid()) << "Workload: " << wload;
        }) << "Workload: " << wload;
    }
}

} // namespace embi
