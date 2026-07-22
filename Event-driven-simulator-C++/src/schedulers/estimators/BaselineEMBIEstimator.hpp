/**
 * @file BaselineEMBIEstimator.hpp
 * @brief Queue-based baseline estimator for EMBI.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#pragma once

#include "schedulers/estimators/EMBIEstimator.hpp"

namespace embi {

/**
 * @class BaselineEMBIEstimator
 * @brief Estimates EMBI using only local queue state: mu_hat * (2*Q + 2*lambda_hat - M).
 */
class BaselineEMBIEstimator : public EMBIEstimator {
public:
    explicit BaselineEMBIEstimator(double M);

    std::vector<double> estimate(const std::vector<Process>& procs,
                                 const GraphState& state) const override;

private:
    double M_;
};

} // namespace embi
