/**
 * @file EMBIScheduler.hpp
 * @brief Scheduler that allocates CPU slices by maximizing estimated EMBI.
 *
 * @author  EMBI Simulator Project
 * @version 2.0.0
 */

#pragma once

#include "schedulers/BaseScheduler.hpp"
#include "schedulers/estimators/EMBIEstimator.hpp"
#include <memory>

namespace embi {

/**
 * @class EMBIScheduler
 * @brief Evaluates estimated EMBI(i) via the configured EMBIEstimator and selects argmax.
 */
class EMBIScheduler final : public BaseScheduler {
public:
    explicit EMBIScheduler(std::unique_ptr<EMBIEstimator> estimator);

    Decision         choose(const SchedulerContext& ctx) override;
    std::string_view name()  const noexcept              override;

private:
    std::unique_ptr<EMBIEstimator> estimator_;
};

} // namespace embi
