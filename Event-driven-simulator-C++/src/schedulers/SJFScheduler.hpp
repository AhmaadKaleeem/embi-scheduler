/**
 * @file SJFScheduler.hpp
 * @brief Shortest Expected Job First (SJF) scheduler based on mu_hat.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#pragma once

#include "schedulers/BaseScheduler.hpp"

namespace embi {

/**
 * @class SJFScheduler
 * @brief Prioritises processes with the highest expected service rate (mu_hat).
 *
 * @par Complexity  O(N) per call to choose().
 */
class SJFScheduler final : public BaseScheduler {
public:
    explicit SJFScheduler(const Config& config);

    Decision         choose(const SchedulerContext& ctx) override;
    std::string_view name()  const noexcept              override;
};

} // namespace embi
