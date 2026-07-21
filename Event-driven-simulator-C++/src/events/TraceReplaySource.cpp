/**
 * @file TraceReplaySource.cpp
 * @brief Implementation of the trace replay source.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#include "events/TraceReplaySource.hpp"

namespace embi {

TraceReplaySource::TraceReplaySource(std::unique_ptr<ReplayEngine> engine)
    : engine_(std::move(engine)) {}

void TraceReplaySource::emitEvents(double tick_d, EventQueue& queue) {
    if (engine_) {
        engine_->emitEvents(tick_d, queue);
    }
}

} // namespace embi
