/**
 * @file CmuScheduler.hpp
 * @brief cμ scheduler: score = μ̂ (service rate only).
 *
 * The cμ (c-mu) rule schedules the process with the highest estimated
 * service rate, ignoring queue length. With unit cost c_i = 1 for all
 * processes, this reduces to scheduling the fastest process.
 *
 * It is optimal for minimising mean number in system under M/G/1 PS
 * assumptions, but can starve slow processes.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#pragma once

#include "schedulers/BaseScheduler.hpp"

namespace embi {

/**
 * @class CmuScheduler
 * @brief cμ rule: score_i = μ̂_i (service rate).
 * @complexity O(N) per choose()
 */
class CmuScheduler final : public BaseScheduler {
public:
    explicit CmuScheduler(const Config& config);

    Decision         choose(const SchedulerContext& ctx) override;
    std::string_view name()  const noexcept              override;
};

} // namespace embi
