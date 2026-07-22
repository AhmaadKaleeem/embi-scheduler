/**
 * @file KatzEMBIEstimator.hpp
 * @brief Katz Centrality implementation for estimating EMBI.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#pragma once

#include "schedulers/estimators/EMBIEstimator.hpp"

namespace embi {

/**
 * @class KatzEMBIEstimator
 * @brief Approximates the latent EMBI using c = (I - alpha A)^-1 b recursively.
 */
class KatzEMBIEstimator : public EMBIEstimator {
public:
    explicit KatzEMBIEstimator(double alpha = 0.5, int max_iterations = 5, double epsilon = 1e-4);

    std::vector<double> estimate(const std::vector<Process>& procs,
                                 const GraphState& state) const override;

private:
    double alpha_;
    int max_iterations_;
    double epsilon_;
};

} // namespace embi
