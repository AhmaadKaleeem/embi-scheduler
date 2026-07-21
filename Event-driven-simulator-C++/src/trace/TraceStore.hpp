/**
 * @file TraceStore.hpp
 * @brief Immutable memory-mapped (or contiguous memory) trace data store.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#pragma once

#include "trace/CanonicalTraceRecord.hpp"
#include <vector>

namespace embi {

class TraceStore {
public:
    explicit TraceStore(std::vector<CanonicalTraceRecord> records);

    [[nodiscard]] const CanonicalTraceRecord* data() const noexcept { return records_.data(); }
    [[nodiscard]] std::size_t size() const noexcept { return records_.size(); }
    [[nodiscard]] bool empty() const noexcept { return records_.empty(); }

    [[nodiscard]] const CanonicalTraceRecord& operator[](std::size_t idx) const {
        return records_[idx];
    }

private:
    std::vector<CanonicalTraceRecord> records_;
};

} // namespace embi
