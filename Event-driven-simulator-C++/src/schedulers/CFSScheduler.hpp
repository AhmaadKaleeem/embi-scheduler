/**
 * @file CFSScheduler.hpp
 * @brief Linux Completely Fair Scheduler (CFS) baseline implementation.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#pragma once

#include "schedulers/BaseScheduler.hpp"
#include <vector>

namespace embi {

/**
 * @class CFSScheduler
 * @brief Prioritises processes with the smallest virtual runtime (vruntime).
 *
 * @par Complexity  O(N) per call to choose().
 */
class CFSScheduler final : public BaseScheduler {
public:
    explicit CFSScheduler(const Config& config);

    Decision         choose(const SchedulerContext& ctx) override;
    std::string_view name()  const noexcept              override;

private:
    std::vector<double> vruntimes_;
    double              min_vruntime_{0.0};
};

} // namespace embi
