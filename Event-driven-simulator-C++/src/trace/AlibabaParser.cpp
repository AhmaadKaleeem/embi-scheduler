/**
 * @file AlibabaParser.cpp
 * @brief Parses Alibaba trace format into canonical format.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#include "trace/AlibabaParser.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iostream>

namespace embi {

std::vector<CanonicalTraceRecord> AlibabaParser::parse(const std::string& filepath) {
    std::vector<CanonicalTraceRecord> records;
    std::ifstream file(filepath);
    
    if (!file.is_open()) {
        throw std::runtime_error("AlibabaParser: Could not open trace file " + filepath);
    }
    
    std::string line;
    // Skip header
    if (!std::getline(file, line)) {
        return records;
    }
    
    // Parse CSV: tick, trace_id, rpc_id, source_service, destination_service, container_id, latency, trace_flags, type, priority
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        
        std::stringstream ss(line);
        std::string token;
        
        CanonicalTraceRecord rec{};
        
        std::getline(ss, token, ','); rec.tick = std::stoull(token);
        std::getline(ss, token, ','); rec.trace_id = std::stoull(token);
        std::getline(ss, token, ','); rec.rpc_id = std::stoull(token);
        std::getline(ss, token, ','); rec.source_service = std::stoul(token);
        std::getline(ss, token, ','); rec.destination_service = std::stoul(token);
        std::getline(ss, token, ','); rec.container_id = std::stoul(token);
        std::getline(ss, token, ','); rec.latency = std::stoul(token);
        std::getline(ss, token, ','); rec.trace_flags = static_cast<uint16_t>(std::stoul(token));
        std::getline(ss, token, ','); rec.type = static_cast<uint8_t>(std::stoul(token));
        std::getline(ss, token, ','); rec.priority = static_cast<uint8_t>(std::stoul(token));
        
        records.push_back(rec);
    }
    
    return records;
}

} // namespace embi
