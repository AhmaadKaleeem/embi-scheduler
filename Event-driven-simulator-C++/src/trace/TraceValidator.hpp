/**
 * @file TraceValidator.hpp
 * @brief Multi-pass validation engine for trace data sets.
 *
 * Enforces rigorous consistency checks required for publication-quality
 * systems research. Performs chronologic, identifier, and bounds checking.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#pragma once

#include "trace/CanonicalTraceRecord.hpp"
#include <vector>
#include <string>
#include <stdexcept>

namespace embi {

class TraceValidationError : public std::runtime_error {
public:
    explicit TraceValidationError(const std::string& msg) : std::runtime_error(msg) {}
};

class TraceValidator {
public:
    /**
     * @brief Performs a 5-pass validation on the trace dataset.
     * @param records The parsed canonical records.
     * @param strict If true, throws on any warning/error. If false, logs warnings.
     * @throws TraceValidationError if strict is true and validation fails.
     */
    static void validate(const std::vector<CanonicalTraceRecord>& records, bool strict = true);

private:
    static void pass1Chronological(const std::vector<CanonicalTraceRecord>& records, bool strict);
    static void pass2IdentifierIntegrity(const std::vector<CanonicalTraceRecord>& records, bool strict);
    static void pass3BoundAnalysis(const std::vector<CanonicalTraceRecord>& records, bool strict);
    static void pass4DistributionFitting(const std::vector<CanonicalTraceRecord>& records, bool strict);
    static void pass5OutlierDetection(const std::vector<CanonicalTraceRecord>& records, bool strict);
};

} // namespace embi
