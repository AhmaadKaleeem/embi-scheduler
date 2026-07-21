/**
 * @file TraceStore.cpp
 * @brief Implementation of the trace store.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#include "trace/TraceStore.hpp"

namespace embi {

TraceStore::TraceStore(std::vector<CanonicalTraceRecord> records)
    : records_(std::move(records)) {}

} // namespace embi
