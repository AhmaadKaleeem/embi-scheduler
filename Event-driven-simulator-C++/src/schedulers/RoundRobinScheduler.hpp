/**
 * @file RoundRobinScheduler.hpp
 * @brief Round-robin scheduler: cycles through processes in index order.
 *
 * Provides a simple baseline that treats all processes equally, regardless
 * of queue length or service rate. Useful for fairness comparisons and as
 * a worst-case scheduling reference.
 *
 * Skips processes with empty queues to avoid idle scheduling.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#pragma once

#include "schedulers/BaseScheduler.hpp"

#include <cstddef>

namespace embi {

/**
 * @class RoundRobinScheduler
 * @brief Round-robin policy: cyclic index with empty-queue skipping.
 * @complexity O(N) per choose() in the worst case (all queues empty except one).
 */
class RoundRobinScheduler final : public BaseScheduler {
public:
    explicit RoundRobinScheduler(const Config& config);

    Decision         choose(const SchedulerContext& ctx) override;
    std::string_view name()  const noexcept              override;

private:
    std::size_t current_index_{0};  ///< Next process to consider in round-robin.
};

} // namespace embi
