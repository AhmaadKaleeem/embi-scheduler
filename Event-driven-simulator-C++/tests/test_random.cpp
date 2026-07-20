/**
 * @file test_random.cpp
 * @brief Unit tests for Random PRNG: seeding, distributions, reproducibility.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#include <gtest/gtest.h>
#include "utils/Random.hpp"

#include <cmath>
#include <limits>
#include <vector>
#include <numeric>

namespace embi {

// ─── Reproducibility ─────────────────────────────────────────────────────────

TEST(RandomTest, SameSeedProducesSameSequence) {
    Random r1(42), r2(42);
    for (int i = 0; i < 1000; ++i) {
        EXPECT_DOUBLE_EQ(r1.uniformReal(0, 1), r2.uniformReal(0, 1));
    }
}

TEST(RandomTest, DifferentSeedsProduceDifferentSequences) {
    Random r1(42), r2(99);
    bool any_different = false;
    for (int i = 0; i < 100; ++i) {
        if (r1.uniformReal(0, 1) != r2.uniformReal(0, 1)) {
            any_different = true;
            break;
        }
    }
    EXPECT_TRUE(any_different);
}

TEST(RandomTest, ReseedRestoresSequence) {
    Random r(42);
    double x1 = r.uniformReal(0, 1);
    r.reseed(42);
    double x2 = r.uniformReal(0, 1);
    EXPECT_DOUBLE_EQ(x1, x2);
}

TEST(RandomTest, CurrentSeedReturnsSeededValue) {
    Random r(12345);
    EXPECT_EQ(r.currentSeed(), 12345ULL);
}

// ─── uniformReal ─────────────────────────────────────────────────────────────

TEST(RandomTest, UniformRealInRange) {
    Random r(42);
    for (int i = 0; i < 10000; ++i) {
        double x = r.uniformReal(2.0, 5.0);
        EXPECT_GE(x, 2.0);
        EXPECT_LT(x, 5.0);
    }
}

TEST(RandomTest, UniformRealInvalidArgThrows) {
    Random r(42);
    EXPECT_THROW(r.uniformReal(5.0, 2.0), std::invalid_argument);
    EXPECT_THROW(r.uniformReal(1.0, 1.0), std::invalid_argument);
}

// ─── uniformInt ──────────────────────────────────────────────────────────────

TEST(RandomTest, UniformIntInRange) {
    Random r(42);
    for (int i = 0; i < 10000; ++i) {
        int64_t x = r.uniformInt(3, 7);
        EXPECT_GE(x, 3);
        EXPECT_LE(x, 7);
    }
}

TEST(RandomTest, UniformIntSingleValue) {
    Random r(42);
    for (int i = 0; i < 100; ++i) {
        EXPECT_EQ(r.uniformInt(5, 5), 5LL);
    }
}

// ─── bernoulli ────────────────────────────────────────────────────────────────

TEST(RandomTest, BernoulliP0AlwaysFalse) {
    Random r(42);
    for (int i = 0; i < 1000; ++i) {
        EXPECT_FALSE(r.bernoulli(0.0));
    }
}

TEST(RandomTest, BernoulliP1AlwaysTrue) {
    Random r(42);
    for (int i = 0; i < 1000; ++i) {
        EXPECT_TRUE(r.bernoulli(1.0));
    }
}

TEST(RandomTest, BernoulliInvalidProbabilityThrows) {
    Random r(42);
    EXPECT_THROW(r.bernoulli(-0.1), std::invalid_argument);
    EXPECT_THROW(r.bernoulli(1.1), std::invalid_argument);
}

TEST(RandomTest, BernoulliP05ApproximatelyHalf) {
    Random r(42);
    int true_count = 0;
    for (int i = 0; i < 100000; ++i) {
        if (r.bernoulli(0.5)) ++true_count;
    }
    double frac = static_cast<double>(true_count) / 100000.0;
    EXPECT_NEAR(frac, 0.5, 0.01);
}

// ─── exponential ─────────────────────────────────────────────────────────────

TEST(RandomTest, ExponentialAllPositive) {
    Random r(42);
    for (int i = 0; i < 10000; ++i) {
        EXPECT_GT(r.exponential(1.0), 0.0);
    }
}

TEST(RandomTest, ExponentialMeanCorrect) {
    Random r(42);
    double rate = 0.5;
    double sum = 0.0;
    constexpr int N = 100000;
    for (int i = 0; i < N; ++i) sum += r.exponential(rate);
    double mean = sum / N;
    EXPECT_NEAR(mean, 1.0 / rate, 0.05 * (1.0 / rate));
}

TEST(RandomTest, ExponentialInvalidRateThrows) {
    Random r(42);
    EXPECT_THROW(r.exponential(0.0), std::invalid_argument);
    EXPECT_THROW(r.exponential(-1.0), std::invalid_argument);
}

// ─── normal ──────────────────────────────────────────────────────────────────

TEST(RandomTest, NormalMeanApproximatelyCorrect) {
    Random r(42);
    double sum = 0.0;
    constexpr int N = 100000;
    for (int i = 0; i < N; ++i) sum += r.normal(5.0, 1.0);
    EXPECT_NEAR(sum / N, 5.0, 0.1);
}

TEST(RandomTest, NormalInvalidStddevThrows) {
    Random r(42);
    EXPECT_THROW(r.normal(0.0, 0.0), std::invalid_argument);
    EXPECT_THROW(r.normal(0.0, -1.0), std::invalid_argument);
}

// ─── pareto ──────────────────────────────────────────────────────────────────

TEST(RandomTest, ParetoAllAtLeastScale) {
    Random r(42);
    double scale = 2.0;
    for (int i = 0; i < 10000; ++i) {
        EXPECT_GE(r.pareto(scale, 2.0), scale);
    }
}

TEST(RandomTest, ParetoInvalidParametersThrow) {
    Random r(42);
    EXPECT_THROW(r.pareto(0.0, 1.5), std::invalid_argument);
    EXPECT_THROW(r.pareto(1.0, 0.0), std::invalid_argument);
    EXPECT_THROW(r.pareto(-1.0, 1.5), std::invalid_argument);
}

// ─── poissonCount ────────────────────────────────────────────────────────────

TEST(RandomTest, PoissonCountZeroMeanReturnsZero) {
    Random r(42);
    for (int i = 0; i < 100; ++i) {
        EXPECT_EQ(r.poissonCount(0.0), 0ULL);
    }
}

TEST(RandomTest, PoissonCountMeanApproximatelyCorrect) {
    Random r(42);
    double mean = 5.0;
    double sum = 0.0;
    constexpr int N = 100000;
    for (int i = 0; i < N; ++i) {
        sum += static_cast<double>(r.poissonCount(mean));
    }
    EXPECT_NEAR(sum / N, mean, 0.1);
}

TEST(RandomTest, PoissonCountNegativeMeanThrows) {
    Random r(42);
    EXPECT_THROW(r.poissonCount(-1.0), std::invalid_argument);
}

} // namespace embi
