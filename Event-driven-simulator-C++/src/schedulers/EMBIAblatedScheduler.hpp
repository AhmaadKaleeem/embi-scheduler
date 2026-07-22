#pragma once

#include "schedulers/BaseScheduler.hpp"
#include "core/Config.hpp"
#include <string_view>

namespace embi {

/**
 * @class EMBIAblatedScheduler
 * @brief Ablation study baseline for EMBI.
 *
 * Removes the arrival-prediction term (lambda) from the EMBI loss function
 * to mathematically prove that predictive awareness is responsible for stability.
 * Score = max(0, mu_hat * (2 * Q - M))
 */
class EMBIAblatedScheduler : public BaseScheduler {
public:
    explicit EMBIAblatedScheduler(const Config& config, bool no_prediction, bool no_penalty);
    
    Decision choose(const SchedulerContext& ctx) override;
    
    [[nodiscard]] std::string_view name() const noexcept override;
    
private:
    double M_;
    bool no_pred_;
    bool no_pen_;
};

} // namespace embi
