/**
 * @file test_workloads.cpp
 * @brief Statistical validation tests for all workload generators.
 *
 * Tests verify that the empirical mean and variance of sampled inter-arrival
 * times are within tolerance of the theoretical values over N=100,000 samples.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#include <gtest/gtest.h>
#include "workloads/UniformWorkload.hpp"
#include "workloads/PoissonWorkload.hpp"
#include "workloads/BurstyWorkload.hpp"
#include "workloads/HeavyTailWorkload.hpp"
#include "workloads/WorkloadProfile.hpp"

#include <cmath>
#include <numeric>
#include <vector>

namespace embi {

namespace {

/// Samples N values and returns {empirical_mean, empirical_variance}.
std::pair<double, double> sampleStats(BaseWorkload& w, std::size_t N) {
    std::vector<double> samples(N);
    for (std::size_t i = 0; i < N; ++i) {
        samples[i] = w.next();
    }

    double sum = std::accumulate(samples.begin(), samples.end(), 0.0);
    double mean = sum / static_cast<double>(N);

    double var = 0.0;
    for (double x : samples) {
        double d = x - mean;
        var += d * d;
    }
    var /= static_cast<double>(N - 1);

    return {mean, var};
}

constexpr std::size_t kSamples = 100'000;
constexpr double kTolerance = 0.05;  // 5% relative tolerance

} // anonymous namespace

// ─── UniformWorkload ──────────────────────────────────────────────────────────

TEST(UniformWorkloadTest, MeanIsCorrect) {
    double lo = 0.5, hi = 2.0;
    UniformWorkload w(42, lo, hi);
    auto [mean, var] = sampleStats(w, kSamples);
    double expected_mean = (lo + hi) / 2.0;
    EXPECT_NEAR(mean, expected_mean, kTolerance * expected_mean);
}

TEST(UniformWorkloadTest, VarianceIsCorrect) {
    double lo = 0.5, hi = 2.0;
    UniformWorkload w(42, lo, hi);
    auto [mean, var] = sampleStats(w, kSamples);
    double range     = hi - lo;
    double expected_var = range * range / 12.0;
    EXPECT_NEAR(var, expected_var, kTolerance * expected_var);
}

TEST(UniformWorkloadTest, SamplesInRange) {
    UniformWorkload w(42, 1.0, 3.0);
    for (int i = 0; i < 10000; ++i) {
        double x = w.next();
        EXPECT_GE(x, 1.0);
        EXPECT_LT(x, 3.0);
    }
}

TEST(UniformWorkloadTest, InvalidParametersFail) {
    EXPECT_THROW(UniformWorkload(42, -1.0, 2.0), std::invalid_argument);
    EXPECT_THROW(UniformWorkload(42, 2.0, 1.0), std::invalid_argument);
    EXPECT_THROW(UniformWorkload(42, 0.0, 2.0), std::invalid_argument);
}

TEST(UniformWorkloadTest, ReproducibleAfterReseed) {
    UniformWorkload w1(42, 0.5, 2.0);
    double x1 = w1.next();
    w1.seed(42);
    double x2 = w1.next();
    EXPECT_DOUBLE_EQ(x1, x2);
}

// ─── PoissonWorkload ──────────────────────────────────────────────────────────

TEST(PoissonWorkloadTest, MeanIsCorrect) {
    double rate = 0.5;
    PoissonWorkload w(42, rate);
    auto [mean, var] = sampleStats(w, kSamples);
    EXPECT_NEAR(mean, 1.0 / rate, kTolerance * (1.0 / rate));
}

TEST(PoissonWorkloadTest, VarianceIsCorrect) {
    double rate = 0.5;
    PoissonWorkload w(42, rate);
    auto [mean, var] = sampleStats(w, kSamples);
    double expected_var = 1.0 / (rate * rate);
    EXPECT_NEAR(var, expected_var, kTolerance * expected_var);
}

TEST(PoissonWorkloadTest, AllSamplesPositive) {
    PoissonWorkload w(42, 0.3);
    for (int i = 0; i < 10000; ++i) {
        EXPECT_GT(w.next(), 0.0);
    }
}

TEST(PoissonWorkloadTest, InvalidRateFails) {
    EXPECT_THROW(PoissonWorkload(42, 0.0), std::invalid_argument);
    EXPECT_THROW(PoissonWorkload(42, -1.0), std::invalid_argument);
}

TEST(PoissonWorkloadTest, ReproducibleAfterReseed) {
    PoissonWorkload w(99, 0.5);
    double x1 = w.next();
    w.seed(99);
    double x2 = w.next();
    EXPECT_DOUBLE_EQ(x1, x2);
}

TEST(PoissonWorkloadTest, NameIsCorrect) {
    PoissonWorkload w(42, 0.5);
    EXPECT_EQ(w.name(), "poisson");
}

// ─── BurstyWorkload ───────────────────────────────────────────────────────────

TEST(BurstyWorkloadTest, EffectiveMeanIsBounded) {
    // With on_rate=0.8, off_rate=0.05, p_on_off=0.1, p_off_on=0.3
    // pi_on = 0.3/(0.1+0.3) = 0.75
    // eff_rate = 0.75*0.8 + 0.25*0.05 = 0.6 + 0.0125 = 0.6125
    // mean = 1/eff_rate ≈ 1.633
    BurstyWorkload w(42, 0.8, 0.05, 0.1, 0.3);
    auto [mean, var] = sampleStats(w, kSamples);
    EXPECT_GT(mean, 0.0);  // sanity: mean must be positive
    EXPECT_LT(mean, 100.0); // sanity: must be finite
}

TEST(BurstyWorkloadTest, InvalidParametersFail) {
    // on_rate out of (0, 1]
    EXPECT_THROW(BurstyWorkload(42, 0.0, 0.1, 0.1, 0.3), std::invalid_argument);
    EXPECT_THROW(BurstyWorkload(42, 1.5, 0.1, 0.1, 0.3), std::invalid_argument);
    // p_on_off out of (0, 1)
    EXPECT_THROW(BurstyWorkload(42, 0.8, 0.05, 0.0, 0.3), std::invalid_argument);
    EXPECT_THROW(BurstyWorkload(42, 0.8, 0.05, 1.0, 0.3), std::invalid_argument);
}

TEST(BurstyWorkloadTest, ReproducibleAfterReseed) {
    BurstyWorkload w(7, 0.8, 0.05, 0.1, 0.3);
    double x1 = w.next();
    w.seed(7);
    double x2 = w.next();
    EXPECT_DOUBLE_EQ(x1, x2);
}

// ─── HeavyTailWorkload ────────────────────────────────────────────────────────

TEST(HeavyTailWorkloadTest, AllSamplesAtLeastScale) {
    double scale = 1.0, shape = 2.0;
    HeavyTailWorkload w(42, scale, shape);
    for (int i = 0; i < 10000; ++i) {
        EXPECT_GE(w.next(), scale);
    }
}

TEST(HeavyTailWorkloadTest, MeanCorrectForAlphaGT1) {
    // For shape > 1: mean = shape * scale / (shape - 1)
    double scale = 1.0, shape = 2.5;
    HeavyTailWorkload w(42, scale, shape);
    auto [mean, var] = sampleStats(w, kSamples);
    double expected_mean = (shape * scale) / (shape - 1.0);
    EXPECT_NEAR(mean, expected_mean, kTolerance * 4.0 * expected_mean);  // heavy tail needs wider tolerance
}

TEST(HeavyTailWorkloadTest, InfiniteMeanReportedForAlphaLE1) {
    HeavyTailWorkload w(42, 1.0, 0.9);  // alpha <= 1 → infinite mean
    EXPECT_EQ(w.mean(), std::numeric_limits<double>::infinity());
}

TEST(HeavyTailWorkloadTest, InvalidParametersFail) {
    EXPECT_THROW(HeavyTailWorkload(42, 0.0, 1.5), std::invalid_argument);
    EXPECT_THROW(HeavyTailWorkload(42, 1.0, 0.0), std::invalid_argument);
    EXPECT_THROW(HeavyTailWorkload(42, -1.0, 1.5), std::invalid_argument);
}

// ─── WorkloadProfile ──────────────────────────────────────────────────────────

TEST(WorkloadProfileTest, AllProfilesBuildSuccessfully) {
    const char* profiles[] = {"web_server", "database", "cloud_vm",
                               "embedded", "real_time", "interactive_desktop"};
    for (const char* p : profiles) {
        EXPECT_NO_THROW({
            auto w = WorkloadProfile::build(p, 42);
            EXPECT_NE(w, nullptr) << "Profile: " << p;
        }) << "Failed for profile: " << p;
    }
}

TEST(WorkloadProfileTest, UnknownProfileFails) {
    EXPECT_THROW((void)WorkloadProfile::build("unknown_profile_xyz", 42), std::invalid_argument);
}

TEST(WorkloadProfileTest, IsKnownProfileWorks) {
    EXPECT_TRUE(WorkloadProfile::isKnownProfile("web_server"));
    EXPECT_FALSE(WorkloadProfile::isKnownProfile("nonexistent"));
}

} // namespace embi
