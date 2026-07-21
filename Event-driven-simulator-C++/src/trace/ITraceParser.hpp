/**
 * @file ITraceParser.hpp
 * @brief Plugin interface for decoding different trace formats into CanonicalTraceRecords.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#pragma once

#include "trace/CanonicalTraceRecord.hpp"
#include <vector>
#include <string>

namespace embi {

/**
 * @class ITraceParser
 * @brief Interface for parsing proprietary trace files into the canonical format.
 */
class ITraceParser {
public:
    virtual ~ITraceParser() = default;

    /**
     * @brief Parses the specified trace file into canonical records.
     * @param filepath Path to the raw trace file.
     * @return Vector of canonical trace records.
     * @throws std::runtime_error on parsing failure.
     */
    virtual std::vector<CanonicalTraceRecord> parse(const std::string& filepath) = 0;
    
    /**
     * @brief Format identifier (e.g. "alibaba")
     */
    [[nodiscard]] virtual std::string format() const noexcept = 0;
};

} // namespace embi
