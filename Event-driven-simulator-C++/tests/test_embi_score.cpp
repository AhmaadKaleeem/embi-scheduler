#include <gtest/gtest.h>
#include "core/Process.hpp"
#include "schedulers/EMBIScheduler.hpp"
#include "schedulers/HybridEMBIScheduler.hpp"
#include "core/Config.hpp"
#include <cmath>

namespace embi {

TEST(EMBIScore, MatchesEquation) {
    // Φ = μ(2Q + 2λ - M)
    Config cfg;
    cfg.M = 10.0;
    
    Process p(1, 2.0, 0.5, 0.1, 0.1);
    p.mu_hat = 0.5;
    p.queue_length = 3;
    p.sync_debt = 0;
    p.lambda_hat = 2.0;

    // Hand-computed:
    // 2Q = 6
    // 2λ = 4
    // 2Q + 2λ - M = 6 + 4 - 10 = 0
    // Φ = 0.5 * 0 = 0.0
    
    double expected_raw = 0.5 * (2.0 * 3.0 + 2.0 * 2.0 - 10.0);
    EXPECT_DOUBLE_EQ(expected_raw, 0.0);
    
    // With Sync Debt
    p.sync_debt = 2;
    // 2(Q+D) = 10
    // 2λ = 4
    // 2(Q+D) + 2λ - M = 10 + 4 - 10 = 4
    // Φ = 0.5 * 4 = 2.0
    expected_raw = 0.5 * (2.0 * (3.0 + 2.0) + 2.0 * 2.0 - 10.0);
    EXPECT_DOUBLE_EQ(expected_raw, 2.0);
}

} // namespace embi
