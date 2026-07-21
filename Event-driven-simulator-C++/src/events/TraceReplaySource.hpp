/**
 * @file TraceReplaySource.hpp
 * @brief IEventSource adapter for ReplayEngine.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#pragma once

#include "events/IEventSource.hpp"
#include "trace/ReplayEngine.hpp"
#include <memory>

namespace embi {

class TraceReplaySource final : public IEventSource {
public:
    explicit TraceReplaySource(std::unique_ptr<ReplayEngine> engine);

    void emitEvents(double tick_d, EventQueue& queue) override;

private:
    std::unique_ptr<ReplayEngine> engine_;
};

} // namespace embi
