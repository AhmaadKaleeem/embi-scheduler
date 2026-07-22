/**
 * @file BaselineEMBIEstimator.cpp
 * @brief Implementation of BaselineEMBIEstimator.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#include "schedulers/estimators/BaselineEMBIEstimator.hpp"
#include <algorithm>

namespace embi {

BaselineEMBIEstimator::BaselineEMBIEstimator(double M) : M_(M) {}

std::vector<double> BaselineEMBIEstimator::estimate(const std::vector<Process>& procs,
                                                    const GraphState&) const {
    const std::size_t N = procs.size();
    std::vector<double> scores(N, 0.0);

    for (std::size_t i = 0; i < N; ++i) {
        const Process& p = procs[i];
        double score = p.mu_hat * (2.0 * static_cast<double>(p.queue_length) + 2.0 * p.lambda_hat - M_);
        scores[i] = std::max(0.0, score);
    }

    return scores;
}

} // namespace embi
