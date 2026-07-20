/**
 * @file test_config.cpp
 * @brief Unit tests for Config validation and ExperimentConfig::expand().
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#include <gtest/gtest.h>
#include "core/Config.hpp"

namespace embi {

// ─── Config validation ────────────────────────────────────────────────────────

TEST(ConfigTest, DefaultConfigValidates) {
    Config cfg;
    EXPECT_NO_THROW(cfg.validate());
}

TEST(ConfigTest, ZeroTicksFails) {
    Config cfg;
    cfg.ticks = 0;
    EXPECT_THROW(cfg.validate(), std::invalid_argument);
}

TEST(ConfigTest, ZeroProcessesFails) {
    Config cfg;
    cfg.num_processes = 0;
    EXPECT_THROW(cfg.validate(), std::invalid_argument);
}

TEST(ConfigTest, InvalidArrivalRateFails) {
    Config cfg;
    cfg.arrival_rate = 0.0;
    EXPECT_THROW(cfg.validate(), std::invalid_argument);

    cfg.arrival_rate = 1.5;
    EXPECT_THROW(cfg.validate(), std::invalid_argument);
}

TEST(ConfigTest, InvalidMFails) {
    Config cfg;
    cfg.M = 0.0;
    EXPECT_THROW(cfg.validate(), std::invalid_argument);

    cfg.M = -1.0;
    EXPECT_THROW(cfg.validate(), std::invalid_argument);
}

TEST(ConfigTest, UnknownSchedulerFails) {
    Config cfg;
    cfg.scheduler_name = "nonexistent_scheduler";
    EXPECT_THROW(cfg.validate(), std::invalid_argument);
}

TEST(ConfigTest, UnknownWorkloadFails) {
    Config cfg;
    cfg.workload_name = "nonexistent_workload";
    EXPECT_THROW(cfg.validate(), std::invalid_argument);
}

TEST(ConfigTest, ValidSchedulerNames) {
    const char* schedulers[] = {"embi", "embi_unclipped", "maxweight", "cmu", "rr", "fcfs"};
    for (const char* sched : schedulers) {
        Config cfg;
        cfg.scheduler_name = sched;
        EXPECT_NO_THROW(cfg.validate()) << "Failed for scheduler: " << sched;
    }
}

TEST(ConfigTest, ValidWorkloadNames) {
    const char* workloads[] = {"uniform", "poisson", "bursty", "heavy_tail"};
    for (const char* wload : workloads) {
        Config cfg;
        cfg.workload_name = wload;
        EXPECT_NO_THROW(cfg.validate()) << "Failed for workload: " << wload;
    }
}

TEST(ConfigTest, TraceWorkloadRequiresTraceFile) {
    Config cfg;
    cfg.workload_name = "trace";
    // No trace_file set
    EXPECT_THROW(cfg.validate(), std::invalid_argument);

    cfg.trace_file = "trace.csv";
    EXPECT_NO_THROW(cfg.validate());
}

TEST(ConfigTest, UniformLoGEHiFails) {
    Config cfg;
    cfg.workload_name = "uniform";
    cfg.uniform_lo    = 2.0;
    cfg.uniform_hi    = 1.0;  // lo >= hi
    EXPECT_THROW(cfg.validate(), std::invalid_argument);
}

TEST(ConfigTest, AlphaOutOfRangeFails) {
    Config cfg;
    cfg.alpha = 0.0;
    EXPECT_THROW(cfg.validate(), std::invalid_argument);

    cfg.alpha = 1.0;
    EXPECT_THROW(cfg.validate(), std::invalid_argument);
}

// ─── Defaults ─────────────────────────────────────────────────────────────────

TEST(ConfigTest, DefaultsMatchDocumentation) {
    Config d = Config::defaults();
    EXPECT_EQ(d.ticks,          1'000'000ULL);
    EXPECT_EQ(d.num_processes,  16UL);
    EXPECT_EQ(d.seed,           42ULL);
    EXPECT_EQ(d.scheduler_name, "embi");
    EXPECT_EQ(d.workload_name,  "poisson");
    EXPECT_DOUBLE_EQ(d.M, 10.0);
    EXPECT_DOUBLE_EQ(d.alpha, 0.1);
    EXPECT_DOUBLE_EQ(d.beta,  0.1);
}

// ─── ExperimentConfig ─────────────────────────────────────────────────────────

TEST(ExperimentConfigTest, TotalRunsCalculation) {
    ExperimentConfig exp;
    exp.schedulers    = {"embi", "maxweight"};   // 2
    exp.workloads     = {"poisson", "bursty"};   // 2
    exp.seeds         = {42, 123, 456};          // 3
    exp.arrival_rates = {0.3, 0.5};             // 2

    EXPECT_EQ(exp.totalRuns(), 2UL * 2UL * 3UL * 2UL);  // = 24
}

TEST(ExperimentConfigTest, ExpandProducesCorrectCount) {
    ExperimentConfig exp;
    exp.schedulers    = {"embi", "rr"};
    exp.workloads     = {"poisson"};
    exp.seeds         = {42};
    exp.arrival_rates = {0.3, 0.5, 0.8};

    auto configs = exp.expand();
    EXPECT_EQ(configs.size(), 2UL * 1UL * 1UL * 3UL);  // = 6
}

TEST(ExperimentConfigTest, ExpandedConfigsAreValid) {
    ExperimentConfig exp;
    exp.schedulers    = {"embi", "fcfs"};
    exp.workloads     = {"poisson"};
    exp.seeds         = {42};
    exp.arrival_rates = {0.5};

    auto configs = exp.expand();
    for (const auto& cfg : configs) {
        EXPECT_NO_THROW(cfg.validate());
    }
}

TEST(ExperimentConfigTest, OutputDirsAreDistinct) {
    ExperimentConfig exp;
    exp.schedulers    = {"embi", "rr"};
    exp.workloads     = {"poisson"};
    exp.seeds         = {42};
    exp.arrival_rates = {0.5};
    exp.output_dir    = "results";

    auto configs = exp.expand();
    ASSERT_EQ(configs.size(), 2UL);
    EXPECT_NE(configs[0].output_dir, configs[1].output_dir);
}

} // namespace embi
