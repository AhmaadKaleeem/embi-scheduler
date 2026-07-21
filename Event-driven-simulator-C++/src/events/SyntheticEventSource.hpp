/**
 * @file SyntheticEventSource.hpp
 * @brief Event source that generates events using statistical distributions.
 *
 * Replaces the workload-sampling logic previously hardcoded in EventLoop.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#pragma once

#include "events/IEventSource.hpp"
#include "core/Config.hpp"
#include "workloads/BaseWorkload.hpp"
#include "workloads/LockContentionWorkload.hpp"
#include <memory>
#include <vector>

namespace embi {

class SyntheticEventSource : public IEventSource {
public:
    SyntheticEventSource(const Config& config, std::unique_ptr<BaseWorkload> workload);

    void emitEvents(double tick_d, EventQueue& queue) override;

    LockContentionWorkload* getLockWorkload() const { return lock_workload_; }

private:
    const Config& config_;
    std::unique_ptr<BaseWorkload> workload_;
    LockContentionWorkload* lock_workload_{nullptr};
    
    std::vector<double> next_arrival_tick_;
    std::vector<double> next_lock_req_tick_;
};

} // namespace embi
