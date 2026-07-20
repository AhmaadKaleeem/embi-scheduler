/**
 * @file test_cli.cpp
 * @brief Unit tests for CLI argument parsing.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#include <gtest/gtest.h>
#include "CLI.hpp"

#include <vector>
#include <string>

namespace embi {

namespace {

/// Converts a vector of strings to argc/argv format.
/// IMPORTANT: argv must outlive this function's call.
std::vector<char*> makeArgv(std::vector<std::string>& args) {
    std::vector<char*> argv;
    argv.reserve(args.size());
    for (auto& s : args) {
        argv.push_back(s.data());
    }
    return argv;
}

} // anonymous namespace

// ─── Default config ───────────────────────────────────────────────────────────

TEST(CLITest, NoArgsUsesDefaults) {
    std::vector<std::string> args = {"embi_sim"};
    auto argv = makeArgv(args);
    auto result = CLI::parse(static_cast<int>(argv.size()), argv.data());

    EXPECT_FALSE(result.experiment_mode);
    EXPECT_FALSE(result.help_requested);
    EXPECT_EQ(result.config.scheduler_name, "embi");
    EXPECT_EQ(result.config.workload_name,  "poisson");
    EXPECT_EQ(result.config.ticks, 1'000'000ULL);
}

// ─── Scheduler flag ───────────────────────────────────────────────────────────

TEST(CLITest, SchedulerFlagParsed) {
    std::vector<std::string> args = {"embi_sim", "--scheduler", "rr"};
    auto argv = makeArgv(args);
    auto result = CLI::parse(static_cast<int>(argv.size()), argv.data());
    EXPECT_EQ(result.config.scheduler_name, "rr");
}

TEST(CLITest, AllSchedulerNamesAccepted) {
    for (const auto& sched : {"embi", "embi_unclipped", "maxweight", "cmu", "rr", "fcfs"}) {
        std::vector<std::string> args = {"embi_sim", "--scheduler", sched};
        auto argv = makeArgv(args);
        EXPECT_NO_THROW((void)CLI::parse(static_cast<int>(argv.size()), argv.data()))
            << "Scheduler: " << sched;
    }
}

// ─── Ticks flag ───────────────────────────────────────────────────────────────

TEST(CLITest, TicksFlagParsed) {
    std::vector<std::string> args = {"embi_sim", "--ticks", "500000"};
    auto argv = makeArgv(args);
    auto result = CLI::parse(static_cast<int>(argv.size()), argv.data());
    EXPECT_EQ(result.config.ticks, 500000ULL);
}

// ─── M flag ───────────────────────────────────────────────────────────────────

TEST(CLITest, MFlagParsed) {
    std::vector<std::string> args = {"embi_sim", "--M", "8.5"};
    auto argv = makeArgv(args);
    auto result = CLI::parse(static_cast<int>(argv.size()), argv.data());
    EXPECT_DOUBLE_EQ(result.config.M, 8.5);
}

// ─── Seed flag ────────────────────────────────────────────────────────────────

TEST(CLITest, SeedFlagParsed) {
    std::vector<std::string> args = {"embi_sim", "--seed", "12345"};
    auto argv = makeArgv(args);
    auto result = CLI::parse(static_cast<int>(argv.size()), argv.data());
    EXPECT_EQ(result.config.seed, 12345ULL);
}

// ─── Arrival rate ─────────────────────────────────────────────────────────────

TEST(CLITest, ArrivalRateFlagParsed) {
    std::vector<std::string> args = {"embi_sim", "--arrival-rate", "0.3"};
    auto argv = makeArgv(args);
    auto result = CLI::parse(static_cast<int>(argv.size()), argv.data());
    EXPECT_DOUBLE_EQ(result.config.arrival_rate, 0.3);
}

// ─── Alpha/Beta ───────────────────────────────────────────────────────────────

TEST(CLITest, AlphaBetaFlagsParsed) {
    std::vector<std::string> args = {"embi_sim", "--alpha", "0.2", "--beta", "0.3"};
    auto argv = makeArgv(args);
    auto result = CLI::parse(static_cast<int>(argv.size()), argv.data());
    EXPECT_DOUBLE_EQ(result.config.alpha, 0.2);
    EXPECT_DOUBLE_EQ(result.config.beta,  0.3);
}

// ─── Logger flags ─────────────────────────────────────────────────────────────

TEST(CLITest, NullLogFlagSetsNullLog) {
    std::vector<std::string> args = {"embi_sim", "--null-log"};
    auto argv = makeArgv(args);
    auto result = CLI::parse(static_cast<int>(argv.size()), argv.data());
    EXPECT_TRUE(result.config.null_log);
}

TEST(CLITest, BinaryLogFlagSetsBinaryLog) {
    std::vector<std::string> args = {"embi_sim", "--binary-log"};
    auto argv = makeArgv(args);
    auto result = CLI::parse(static_cast<int>(argv.size()), argv.data());
    EXPECT_TRUE(result.config.binary_log);
}

// ─── Help ─────────────────────────────────────────────────────────────────────

TEST(CLITest, HelpFlagSetsHelpRequested) {
    std::vector<std::string> args = {"embi_sim", "--help"};
    auto argv = makeArgv(args);
    auto result = CLI::parse(static_cast<int>(argv.size()), argv.data());
    EXPECT_TRUE(result.help_requested);
}

TEST(CLITest, ShortHFlagSetsHelpRequested) {
    std::vector<std::string> args = {"embi_sim", "-h"};
    auto argv = makeArgv(args);
    auto result = CLI::parse(static_cast<int>(argv.size()), argv.data());
    EXPECT_TRUE(result.help_requested);
}

// ─── Unknown flags ────────────────────────────────────────────────────────────

TEST(CLITest, UnknownFlagThrows) {
    std::vector<std::string> args = {"embi_sim", "--unknown-flag"};
    auto argv = makeArgv(args);
    EXPECT_THROW((void)CLI::parse(static_cast<int>(argv.size()), argv.data()),
                 std::invalid_argument);
}

// ─── Missing value ────────────────────────────────────────────────────────────

TEST(CLITest, MissingValueForSchedulerThrows) {
    std::vector<std::string> args = {"embi_sim", "--scheduler"};
    auto argv = makeArgv(args);
    EXPECT_THROW((void)CLI::parse(static_cast<int>(argv.size()), argv.data()),
                 std::invalid_argument);
}

// ─── No-clip ──────────────────────────────────────────────────────────────────

TEST(CLITest, NoClipFlagDisablesClipping) {
    std::vector<std::string> args = {"embi_sim", "--no-clip"};
    auto argv = makeArgv(args);
    auto result = CLI::parse(static_cast<int>(argv.size()), argv.data());
    EXPECT_FALSE(result.config.embi_clipped);
}

// ─── Output dir ───────────────────────────────────────────────────────────────

TEST(CLITest, OutputFlagParsed) {
    std::vector<std::string> args = {"embi_sim", "--output", "my_results"};
    auto argv = makeArgv(args);
    auto result = CLI::parse(static_cast<int>(argv.size()), argv.data());
    EXPECT_EQ(result.config.output_dir, "my_results");
}

// ─── Log freq ────────────────────────────────────────────────────────────────

TEST(CLITest, LogFreqFlagParsed) {
    std::vector<std::string> args = {"embi_sim", "--log-freq", "100"};
    auto argv = makeArgv(args);
    auto result = CLI::parse(static_cast<int>(argv.size()), argv.data());
    EXPECT_EQ(result.config.log_freq, 100ULL);
}

// ─── Multiple flags ───────────────────────────────────────────────────────────

TEST(CLITest, MultipleValidFlagsAllApplied) {
    std::vector<std::string> args = {
        "embi_sim",
        "--scheduler", "maxweight",
        "--workload",  "bursty",
        "--ticks",     "50000",
        "--seed",      "99",
        "--M",         "5.0",
        "--null-log"
    };
    auto argv = makeArgv(args);
    auto result = CLI::parse(static_cast<int>(argv.size()), argv.data());

    EXPECT_EQ(result.config.scheduler_name, "maxweight");
    EXPECT_EQ(result.config.workload_name,  "bursty");
    EXPECT_EQ(result.config.ticks,          50000ULL);
    EXPECT_EQ(result.config.seed,           99ULL);
    EXPECT_DOUBLE_EQ(result.config.M, 5.0);
    EXPECT_TRUE(result.config.null_log);
}

} // namespace embi
