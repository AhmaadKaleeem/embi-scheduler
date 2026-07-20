/**
 * @file test_statistics_db.cpp
 * @brief Unit tests for StatisticsDatabase and Statistics accumulator.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#include <gtest/gtest.h>
#include "logging/StatisticsDatabase.hpp"
#include "logging/NullLogger.hpp"
#include "logging/Statistics.hpp"

#include <cmath>

namespace embi {

// ─── Statistics ───────────────────────────────────────────────────────────────

TEST(StatisticsTest, RecordAndRetrieve) {
    Statistics s;
    s.record("jain", 0.9);
    s.record("jain", 0.8);
    s.record("jain", 0.7);

    auto [mean, std] = s.meanStdDev("jain");
    EXPECT_NEAR(mean, 0.8, 1e-9);
    EXPECT_GT(std, 0.0);
}

TEST(StatisticsTest, EmptyMetricReturnsZeroMeanStd) {
    Statistics s;
    auto [mean, std] = s.meanStdDev("nonexistent");
    EXPECT_DOUBLE_EQ(mean, 0.0);
    EXPECT_DOUBLE_EQ(std, 0.0);
}

TEST(StatisticsTest, SamplesReturnsCorrectValues) {
    Statistics s;
    s.record("x", 1.0);
    s.record("x", 2.0);
    s.record("x", 3.0);

    const auto& v = s.samples("x");
    ASSERT_EQ(v.size(), 3UL);
    EXPECT_DOUBLE_EQ(v[0], 1.0);
    EXPECT_DOUBLE_EQ(v[1], 2.0);
    EXPECT_DOUBLE_EQ(v[2], 3.0);
}

TEST(StatisticsTest, EmptySamplesReturnsEmptyVector) {
    Statistics s;
    const auto& v = s.samples("missing");
    EXPECT_TRUE(v.empty());
}

TEST(StatisticsTest, MetricNamesAreSorted) {
    Statistics s;
    s.record("zebra",   1.0);
    s.record("apple",   2.0);
    s.record("mango",   3.0);

    auto names = s.metricNames();
    ASSERT_EQ(names.size(), 3UL);
    EXPECT_EQ(names[0], "apple");
    EXPECT_EQ(names[1], "mango");
    EXPECT_EQ(names[2], "zebra");
}

TEST(StatisticsTest, TotalSamplesCorrect) {
    Statistics s;
    s.record("a", 1.0);
    s.record("a", 2.0);
    s.record("b", 3.0);
    EXPECT_EQ(s.totalSamples(), 3UL);
}

TEST(StatisticsTest, ResetClearsAllMetrics) {
    Statistics s;
    s.record("x", 5.0);
    s.reset();
    EXPECT_TRUE(s.metricNames().empty());
    EXPECT_EQ(s.totalSamples(), 0UL);
}

TEST(StatisticsTest, SingleSampleStdDevIsZero) {
    Statistics s;
    s.record("x", 42.0);
    auto [mean, std] = s.meanStdDev("x");
    EXPECT_DOUBLE_EQ(mean, 42.0);
    EXPECT_DOUBLE_EQ(std, 0.0);
}

// ─── StatisticsDatabase ───────────────────────────────────────────────────────

namespace {
Config makeNullLogConfig() {
    Config cfg;
    cfg.null_log       = true;
    cfg.scheduler_name = "embi";
    cfg.workload_name  = "poisson";
    cfg.ticks          = 1000;
    cfg.num_processes  = 4;
    return cfg;
}
} // anonymous namespace

TEST(StatisticsDatabaseTest, CreateWithNullLoggerSucceeds) {
    Config cfg = makeNullLogConfig();
    EXPECT_NO_THROW({
        auto db = StatisticsDatabase::create(cfg);
        EXPECT_NE(db, nullptr);
        EXPECT_EQ(db->loggerType(), "null");
    });
}

TEST(StatisticsDatabaseTest, RecordStatAndRetrieve) {
    auto db = StatisticsDatabase::create(makeNullLogConfig());
    db->recordStat("latency", 5.0);
    db->recordStat("latency", 7.0);

    auto [mean, std] = db->stats().meanStdDev("latency");
    EXPECT_NEAR(mean, 6.0, 1e-9);
}

TEST(StatisticsDatabaseTest, RecordDoesNotThrow) {
    auto db = StatisticsDatabase::create(makeNullLogConfig());

    LogRecord r;
    r.tick         = 10;
    r.pid          = 0;
    r.queue_length = 3;
    r.throughput   = 0.5;

    EXPECT_NO_THROW(db->record(r));
}

TEST(StatisticsDatabaseTest, FlushDoesNotThrow) {
    auto db = StatisticsDatabase::create(makeNullLogConfig());
    EXPECT_NO_THROW(db->flush());
}

TEST(StatisticsDatabaseTest, CloseDoesNotThrow) {
    auto db = StatisticsDatabase::create(makeNullLogConfig());
    EXPECT_NO_THROW(db->close());
}

TEST(StatisticsDatabaseTest, DoubleCloseDoesNotThrow) {
    auto db = StatisticsDatabase::create(makeNullLogConfig());
    db->close();
    EXPECT_NO_THROW(db->close());  // second close must be safe
}

} // namespace embi
