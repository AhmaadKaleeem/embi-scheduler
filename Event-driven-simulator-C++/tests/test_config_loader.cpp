/**
 * @file test_config_loader.cpp
 * @brief Unit tests for ConfigLoader: YAML and JSON file parsing.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#include <gtest/gtest.h>
#include "utils/ConfigLoader.hpp"
#include "utils/FileUtils.hpp"

#include <fstream>
#include <filesystem>
#include <string>

namespace embi {

namespace {

/// Writes a string to a temporary file and returns the path.
std::string writeTempFile(const std::string& filename, const std::string& content) {
    auto path = (std::filesystem::temp_directory_path() / filename).string();
    std::ofstream f(path);
    if (!f.is_open()) {
        throw std::runtime_error("writeTempFile: cannot create " + path);
    }
    f << content;
    return path;
}

} // anonymous namespace

// ─── YAML loading ────────────────────────────────────────────────────────────

TEST(ConfigLoaderTest, LoadValidYAMLProducesValidConfig) {
    std::string content =
        "scheduler: maxweight\n"
        "workload: poisson\n"
        "ticks: 50000\n"
        "num_processes: 8\n"
        "seed: 77\n"
        "arrival_rate: 0.3\n"
        "service_rate: 1.0\n"
        "alpha: 0.2\n"
        "beta: 0.15\n"
        "M: 5.0\n"
        "output_dir: test_output\n";

    std::string path = writeTempFile("test_config.yaml", content);
    ASSERT_NO_THROW({
        Config cfg = ConfigLoader::load(path);
        EXPECT_EQ(cfg.scheduler_name, "maxweight");
        EXPECT_EQ(cfg.workload_name,  "poisson");
        EXPECT_EQ(cfg.ticks,          50000ULL);
        EXPECT_EQ(cfg.num_processes,  8UL);
        EXPECT_EQ(cfg.seed,           77ULL);
        EXPECT_DOUBLE_EQ(cfg.arrival_rate, 0.3);
        EXPECT_DOUBLE_EQ(cfg.M, 5.0);
        EXPECT_DOUBLE_EQ(cfg.alpha, 0.2);
        EXPECT_DOUBLE_EQ(cfg.beta, 0.15);
    });
    std::filesystem::remove(path);
}

TEST(ConfigLoaderTest, YAMLCommentLinesIgnored) {
    std::string content =
        "# This is a comment\n"
        "scheduler: rr\n"
        "workload: uniform\n"
        "uniform_lo: 0.5\n"
        "uniform_hi: 2.0\n"
        "# Another comment\n"
        "ticks: 10000\n";

    std::string path = writeTempFile("test_comments.yaml", content);
    ASSERT_NO_THROW({
        Config cfg = ConfigLoader::load(path);
        EXPECT_EQ(cfg.scheduler_name, "rr");
        EXPECT_EQ(cfg.workload_name, "uniform");
        EXPECT_EQ(cfg.ticks, 10000ULL);
    });
    std::filesystem::remove(path);
}

TEST(ConfigLoaderTest, YAMLUnknownSchedulerThrowsOnValidation) {
    std::string content = "scheduler: bad_scheduler\nworkload: poisson\n";
    std::string path = writeTempFile("test_bad.yaml", content);
    EXPECT_THROW((void)ConfigLoader::load(path), std::invalid_argument);
    std::filesystem::remove(path);
}

// ─── JSON loading ─────────────────────────────────────────────────────────────

TEST(ConfigLoaderTest, LoadValidJSONProducesValidConfig) {
    std::string content = R"({
        "scheduler": "fcfs",
        "workload": "bursty",
        "ticks": 200000,
        "num_processes": 16,
        "seed": 99,
        "arrival_rate": 0.5,
        "service_rate": 1.0,
        "alpha": 0.1,
        "beta": 0.1,
        "M": 10.0,
        "burst_on_rate": 0.8,
        "burst_off_rate": 0.05,
        "burst_p_on_off": 0.1,
        "burst_p_off_on": 0.3
    })";

    std::string path = writeTempFile("test_config.json", content);
    ASSERT_NO_THROW({
        Config cfg = ConfigLoader::load(path);
        EXPECT_EQ(cfg.scheduler_name, "fcfs");
        EXPECT_EQ(cfg.workload_name,  "bursty");
        EXPECT_EQ(cfg.ticks, 200000ULL);
        EXPECT_DOUBLE_EQ(cfg.burst_on_rate, 0.8);
    });
    std::filesystem::remove(path);
}

TEST(ConfigLoaderTest, LoadJSONExperimentConfig) {
    std::string content = R"({
        "schedulers":    ["embi", "rr"],
        "workloads":     ["poisson"],
        "seeds":         [42, 123],
        "arrival_rates": [0.3, 0.5],
        "ticks":         100000,
        "num_processes": 8,
        "M":             10.0,
        "service_rate":  1.0,
        "alpha":         0.1,
        "beta":          0.1
    })";

    std::string path = writeTempFile("test_experiment.json", content);
    ASSERT_NO_THROW({
        ExperimentConfig exp = ConfigLoader::loadExperiment(path);
        EXPECT_EQ(exp.schedulers.size(), 2UL);
        EXPECT_EQ(exp.workloads.size(),  1UL);
        EXPECT_EQ(exp.seeds.size(),      2UL);
        EXPECT_EQ(exp.arrival_rates.size(), 2UL);
        EXPECT_EQ(exp.totalRuns(), 2UL * 1UL * 2UL * 2UL);  // = 8
    });
    std::filesystem::remove(path);
}

// ─── Error cases ─────────────────────────────────────────────────────────────

TEST(ConfigLoaderTest, LoadNonExistentFileThrows) {
    EXPECT_THROW((void)ConfigLoader::load("/nonexistent/path/config.yaml"),
                 std::runtime_error);
}

TEST(ConfigLoaderTest, LoadUnsupportedExtensionThrows) {
    std::string content = "scheduler: embi\n";
    std::string path = writeTempFile("test_config.toml", content);
    EXPECT_THROW((void)ConfigLoader::load(path), std::invalid_argument);
    std::filesystem::remove(path);
}

TEST(ConfigLoaderTest, LoadExperimentFromYAMLThrows) {
    std::string content = "scheduler: embi\n";
    std::string path = writeTempFile("test_config2.yaml", content);
    EXPECT_THROW((void)ConfigLoader::loadExperiment(path), std::invalid_argument);
    std::filesystem::remove(path);
}

TEST(ConfigLoaderTest, LoadMalformedJSONThrows) {
    std::string content = "{ this is not valid json !!!";
    std::string path = writeTempFile("test_bad.json", content);
    EXPECT_THROW((void)ConfigLoader::load(path), std::runtime_error);
    std::filesystem::remove(path);
}

} // namespace embi
