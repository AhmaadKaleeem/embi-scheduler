/**
 * @file Statistics.cpp
 * @brief Statistics accumulator implementation.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#include "logging/Statistics.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>

namespace embi {

const std::vector<double> Statistics::kEmpty_;

void Statistics::record(const std::string& name, double value) {
    data_[name].push_back(value);
}

std::pair<double, double> Statistics::meanStdDev(const std::string& name) const {
    auto it = data_.find(name);
    if (it == data_.end() || it->second.empty()) return {0.0, 0.0};

    const auto& v = it->second;
    double sum = std::accumulate(v.begin(), v.end(), 0.0);
    double mean = sum / static_cast<double>(v.size());

    double variance = 0.0;
    for (double x : v) {
        double diff = x - mean;
        variance += diff * diff;
    }
    variance /= static_cast<double>(v.size());
    return {mean, std::sqrt(variance)};
}

const std::vector<double>& Statistics::samples(const std::string& name) const {
    auto it = data_.find(name);
    return (it != data_.end()) ? it->second : kEmpty_;
}

std::vector<std::string> Statistics::metricNames() const {
    std::vector<std::string> names;
    names.reserve(data_.size());
    for (const auto& kv : data_) {
        names.push_back(kv.first);
    }
    std::sort(names.begin(), names.end());
    return names;
}

std::size_t Statistics::totalSamples() const noexcept {
    std::size_t total = 0;
    for (const auto& kv : data_) {
        total += kv.second.size();
    }
    return total;
}

void Statistics::reset() {
    data_.clear();
}

} // namespace embi
