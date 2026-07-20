/**
 * @file FCFSScheduler.hpp
 * @brief First-Come-First-Served scheduler: serves the process with the
 *        earliest first arrival time.
 *
 * FCFS provides a fairness baseline: processes that have been waiting
 * longest are prioritised. It does not consider queue length or service rate.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#pragma once

#include "schedulers/BaseScheduler.hpp"

namespace embi {

/**
 * @class FCFSScheduler
 * @brief FCFS policy: schedules the process with the earliest first_arrival_time.
 *        Breaks ties by process ID (lower ID first).
 * @complexity O(N) per choose()
 */
class FCFSScheduler final : public BaseScheduler {
public:
    explicit FCFSScheduler(const Config& config);

    Decision         choose(const SchedulerContext& ctx) override;
    std::string_view name()  const noexcept              override;
};

} // namespace embi
