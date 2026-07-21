/**
 * @file ReplayEngine.hpp
 * @brief Engine for generating simulation events from a TraceStore.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#pragma once

#include "trace/TraceStore.hpp"
#include "trace/TraceConfig.hpp"
#include "core/EventQueue.hpp"
#include <memory>

namespace embi {

class ReplayEngine {
public:
    ReplayEngine(std::shared_ptr<const TraceStore> store, TraceConfig config);

    /**
     * @brief Pushes events belonging to the current tick into the EventQueue.
     * @param tick_d Current simulation tick (double).
     * @param queue EventQueue to populate.
     */
    void emitEvents(double tick_d, EventQueue& queue);

    [[nodiscard]] bool isDone() const noexcept { return current_idx_ >= store_->size(); }

private:
    std::shared_ptr<const TraceStore> store_;
    TraceConfig                       config_;
    std::size_t                       current_idx_{0};
};

} // namespace embi
