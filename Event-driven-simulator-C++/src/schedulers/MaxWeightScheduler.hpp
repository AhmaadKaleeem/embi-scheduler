/**
 * @file MaxWeightScheduler.hpp
 * @brief MaxWeight scheduler: score = μ̂·Q.
 *
 * The MaxWeight policy selects the process that maximises the product of
 * estimated service rate and queue length. It is throughput-optimal for
 * independent queues and forms the baseline against which EMBI is compared.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#pragma once

#include "schedulers/BaseScheduler.hpp"

namespace embi {

/**
 * @class MaxWeightScheduler
 * @brief MaxWeight scheduling policy: score_i = μ̂_i · Q_i.
 * @complexity O(N) per choose()
 */
class MaxWeightScheduler final : public BaseScheduler {
public:
    explicit MaxWeightScheduler(const Config& config);

    Decision         choose(const SchedulerContext& ctx) override;
    std::string_view name()  const noexcept              override;

private:
    // Config stored for potential future extensions (currently unused)
    double alpha_;
    double beta_;
};

} // namespace embi
