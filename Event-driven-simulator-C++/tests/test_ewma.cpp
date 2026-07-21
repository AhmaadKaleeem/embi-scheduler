#include <gtest/gtest.h>
#include "core/Process.hpp"
#include <cmath>

namespace embi {

TEST(EWMATest, MatchesEquation) {
    double alpha = 0.1;
    double beta = 0.2;
    Process p(1, 0.0, 0.0, alpha, beta); // id, true_lambda, true_mu, alpha, beta
    
    // Initial state
    EXPECT_DOUBLE_EQ(p.lambda_hat, 0.0);
    EXPECT_DOUBLE_EQ(p.mu_hat, 0.0);
    
    // arrival EWMA: lambda_hat = (1-alpha)*lambda_hat + alpha*x
    // If x=1:
    p.updateArrivalEstimate(1.0); // 1.0 ticks -> rate 1.0
    double expected_lambda = 0.1 * 1.0 + 0.9 * 0.0;
    EXPECT_DOUBLE_EQ(p.lambda_hat, expected_lambda);
    
    // next arrival (inter-arrival 2.0 ticks -> rate 0.5)
    p.updateArrivalEstimate(2.0);
    expected_lambda = 0.1 * 0.5 + 0.9 * expected_lambda;
    EXPECT_DOUBLE_EQ(p.lambda_hat, expected_lambda);

    // service EWMA: mu_hat = (1-beta)*mu_hat + beta*y
    p.updateServiceEstimate(1.0); // 1.0 ticks -> rate 1.0
    double expected_mu = 0.2 * 1.0 + 0.8 * 0.0;
    EXPECT_DOUBLE_EQ(p.mu_hat, expected_mu);
}

} // namespace embi
