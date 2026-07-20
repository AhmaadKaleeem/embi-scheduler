/**
 * @file TraceLoader.cpp
 * @brief Implementation of TraceLoader: CSV trace file parsing and replay.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#include "workloads/TraceLoader.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <numeric>
#include <sstream>
#include <stdexcept>

namespace embi {

// ─── Construction ────────────────────────────────────────────────────────────

TraceLoader::TraceLoader(const std::string& path, bool looping)
    : looping_(looping)
{
    loadFile(path);
    if (records_.empty()) {
        throw std::invalid_argument(
            "TraceLoader: no valid records found in '" + path + "'");
    }
    // Sort by tick to guarantee chronological order
    std::sort(records_.begin(), records_.end(),
              [](const TraceRecord& a, const TraceRecord& b) {
                  return a.tick < b.tick;
              });
    computeStatistics();
    last_tick_ = static_cast<double>(records_.front().tick);
}

// ─── BaseWorkload interface ───────────────────────────────────────────────────

double TraceLoader::next() {
    if (records_.empty()) return 1.0e12;

    if (index_ >= records_.size()) {
        if (looping_) {
            index_     = 0;
            last_tick_ = static_cast<double>(records_.front().tick);
        } else {
            // Trace exhausted; return a large inter-arrival (no more arrivals)
            return 1.0e12;
        }
    }

    const TraceRecord& rec = records_[index_];
    double inter_arrival   = static_cast<double>(rec.tick) - last_tick_;
    last_tick_             = static_cast<double>(rec.tick);
    index_++;

    // Clamp: inter-arrival must be non-negative
    return std::max(0.0, inter_arrival);
}

void TraceLoader::seed(uint64_t /*s*/) {
    // Re-seed for TraceLoader means reset playback position
    index_     = 0;
    last_tick_ = (records_.empty()) ? 0.0 : static_cast<double>(records_.front().tick);
}

std::string_view TraceLoader::name() const noexcept {
    return "trace";
}

double TraceLoader::mean() const noexcept {
    return mean_;
}

double TraceLoader::variance() const noexcept {
    return variance_;
}

std::size_t TraceLoader::recordCount() const noexcept {
    return records_.size();
}

const std::vector<TraceRecord>& TraceLoader::records() const noexcept {
    return records_;
}

std::size_t TraceLoader::currentIndex() const noexcept {
    return index_;
}

// ─── Private helpers ─────────────────────────────────────────────────────────

void TraceLoader::loadFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error(
            "TraceLoader: cannot open trace file '" + path + "'");
    }

    records_.clear();
    std::string line;
    bool header_skipped = false;

    while (std::getline(file, line)) {
        // Skip blank lines and comments
        if (line.empty() || line[0] == '#') continue;

        // Skip header row (contains "tick" as first token)
        if (!header_skipped) {
            // Check if the first field is "tick" (case-insensitive)
            std::string first;
            std::istringstream check(line);
            std::getline(check, first, ',');
            // Trim whitespace
            while (!first.empty() && (first.front() == ' ' || first.front() == '\r'))
                first.erase(first.begin());
            while (!first.empty() && (first.back() == ' ' || first.back() == '\r'))
                first.pop_back();

            if (first == "tick" || first == "Tick" || first == "TICK") {
                header_skipped = true;
                continue;
            }
            header_skipped = true;  // No header, parse this as data
        }

        // Parse CSV columns: tick,pid,arrival,service,priority,deadline
        std::istringstream ss(line);
        std::string token;
        std::vector<std::string> cols;

        while (std::getline(ss, token, ',')) {
            // Trim whitespace and carriage returns
            while (!token.empty() &&
                   (token.front() == ' ' || token.front() == '\r' || token.front() == '\t'))
                token.erase(token.begin());
            while (!token.empty() &&
                   (token.back() == ' ' || token.back() == '\r' || token.back() == '\t'))
                token.pop_back();
            cols.push_back(token);
        }

        if (cols.empty()) continue;

        TraceRecord rec;

        try {
            if (cols.size() > 0) rec.tick          = static_cast<uint64_t>(std::stoull(cols[0]));
            if (cols.size() > 1) rec.pid            = static_cast<std::size_t>(std::stoull(cols[1]));
            if (cols.size() > 2) rec.arrival_count  = static_cast<uint64_t>(std::stoull(cols[2]));
            if (cols.size() > 3) rec.service_time   = std::stod(cols[3]);
            if (cols.size() > 4 && !cols[4].empty()) rec.priority = std::stod(cols[4]);
            if (cols.size() > 5 && !cols[5].empty()) rec.deadline = std::stod(cols[5]);
        } catch (const std::exception&) {
            // Skip malformed rows silently (research traces often have noise)
            continue;
        }

        records_.push_back(rec);
    }
}

void TraceLoader::computeStatistics() {
    if (records_.size() < 2) {
        mean_     = 1.0;
        variance_ = 0.0;
        return;
    }

    // Compute inter-arrival times between consecutive records
    std::vector<double> inter_arrivals;
    inter_arrivals.reserve(records_.size() - 1);

    for (std::size_t i = 1; i < records_.size(); ++i) {
        double dt = static_cast<double>(records_[i].tick) -
                    static_cast<double>(records_[i - 1].tick);
        if (dt >= 0.0) inter_arrivals.push_back(dt);
    }

    if (inter_arrivals.empty()) {
        mean_     = 1.0;
        variance_ = 0.0;
        return;
    }

    // Mean
    double sum = std::accumulate(inter_arrivals.begin(), inter_arrivals.end(), 0.0);
    mean_      = sum / static_cast<double>(inter_arrivals.size());

    // Variance (Welford's online algorithm for numerical stability)
    double m2 = 0.0;
    double m  = 0.0;
    for (std::size_t i = 0; i < inter_arrivals.size(); ++i) {
        double delta  = inter_arrivals[i] - m;
        m            += delta / static_cast<double>(i + 1);
        double delta2 = inter_arrivals[i] - m;
        m2           += delta * delta2;
    }
    variance_ = (inter_arrivals.size() > 1)
                    ? m2 / static_cast<double>(inter_arrivals.size() - 1)
                    : 0.0;
}

} // namespace embi
