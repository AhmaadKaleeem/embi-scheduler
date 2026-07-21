/**
 * @file AlibabaParser.hpp
 * @brief Parses Alibaba trace format (CSV) into canonical format.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#pragma once

#include "trace/ITraceParser.hpp"

namespace embi {

class AlibabaParser final : public ITraceParser {
public:
    std::vector<CanonicalTraceRecord> parse(const std::string& filepath) override;
    
    [[nodiscard]] std::string format() const noexcept override { return "alibaba"; }
};

} // namespace embi
