#pragma once

#include "schedulers/BaseScheduler.hpp"
#include <string_view>

namespace embi {

/**
 * @class GSQScheduler
 * @brief Greedy Shortest Queue baseline.
 *
 * Selects the process with the absolute shortest queue length.
 */
class GSQScheduler : public BaseScheduler {
public:
    GSQScheduler() = default;
    
    Decision choose(const SchedulerContext& ctx) override;
    
    [[nodiscard]] std::string_view name() const noexcept override;
};

} // namespace embi
