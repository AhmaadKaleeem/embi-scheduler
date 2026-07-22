/**
 * @file EMBIEstimator.hpp
 * @brief Interface for EMBI (Estimated Marginal Blocking Impact) estimators.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#pragma once

#include "core/GraphState.hpp"
#include "core/Process.hpp"
#include <vector>

namespace embi {

/**
 * @class EMBIEstimator
 * @brief Base interface for estimators that approximate the latent EMBI quantity.
 *        EMBI(i) = J(\pi) - J(\pi_i)
 */
class EMBIEstimator {
public:
    virtual ~EMBIEstimator() = default;

    /**
     * @brief Estimates the marginal impact for all processes given observable info.
     * @param procs The active simulation processes.
     * @param state The global graph state (topology + dynamic state).
     * @return A vector of \hat{EMBI}_i values, where index is process id.
     */
    virtual std::vector<double> estimate(const std::vector<Process>& procs,
                                         const GraphState& state) const = 0;
};

} // namespace embi
