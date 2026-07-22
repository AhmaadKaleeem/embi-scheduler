/**
 * @file KatzEMBIEstimator.cpp
 * @brief Implementation of Katz EMBI Estimator.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#include "schedulers/estimators/KatzEMBIEstimator.hpp"
#include <cmath>
#include <algorithm>

namespace embi {

KatzEMBIEstimator::KatzEMBIEstimator(double alpha, int max_iterations, double epsilon)
    : alpha_(alpha), max_iterations_(max_iterations), epsilon_(epsilon) {}

std::vector<double> KatzEMBIEstimator::estimate(const std::vector<Process>& procs,
                                                const GraphState& state) const {
    const std::size_t N = procs.size();
    std::vector<double> b(N, 0.0);
    std::vector<double> c(N, 0.0);

    // 1. Calculate local impact b_i for each process
    for (std::size_t i = 0; i < N; ++i) {
        b[i] = static_cast<double>(procs[i].queue_length + procs[i].sync_debt);
        c[i] = b[i]; // Initial guess c^(0) = b
    }

    if (!state.topology) {
        return c; // No topology available, fall back to local impact
    }

    // 2. Iterate Katz Centrality
    std::vector<double> c_next(N, 0.0);
    for (int iter = 0; iter < max_iterations_; ++iter) {
        double max_diff = 0.0;
        for (std::size_t i = 0; i < N; ++i) {
            double sum_A_cj = 0.0;
            auto it = state.topology->service_graph.find(i);
            if (it != state.topology->service_graph.end()) {
                for (uint32_t dep_id : it->second) {
                    if (dep_id < N) {
                        sum_A_cj += c[dep_id];
                    }
                }
            }
            double new_c = alpha_ * sum_A_cj + b[i];
            max_diff = std::max(max_diff, std::abs(new_c - c[i]));
            c_next[i] = new_c;
        }
        c = c_next;
        if (max_diff < epsilon_) {
            break;
        }
    }

    return c;
}

} // namespace embi
