/**
 * @file SummaryWriter.cpp
 * @brief SummaryWriter implementation.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#include "logging/SummaryWriter.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace embi {

namespace {

/// Fixed-width column formatter helper.
std::string col(const std::string& s, int width) {
    std::ostringstream oss;
    oss << std::left << std::setw(width) << s;
    return oss.str();
}

std::string fmtDouble(double v, int prec = 4) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(prec) << v;
    return oss.str();
}

} // anonymous namespace

void SummaryWriter::writeComparativeTable(const std::string&             path,
                                           const std::vector<RunSummary>& summaries) const {
    std::ofstream f(path);
    if (!f.is_open()) {
        throw std::runtime_error("SummaryWriter: cannot write '" + path + "'");
    }

    // Sort by Jain Fairness Index descending
    std::vector<const RunSummary*> sorted;
    sorted.reserve(summaries.size());
    for (const auto& s : summaries) sorted.push_back(&s);
    std::sort(sorted.begin(), sorted.end(),
              [](const RunSummary* a, const RunSummary* b) {
                  return a->offline.jain_fairness_index > b->offline.jain_fairness_index;
              });

    f << "╔══════════════════════════════════════════════════════════════════╗\n"
      << "║       EMBI Experiment Comparative Summary                        ║\n"
      << "╚══════════════════════════════════════════════════════════════════╝\n\n";

    // Header
    f << col("Scheduler",  14)
      << col("Workload",   12)
      << col("Rate",       8)
      << col("AvgWait",    10)
      << col("P99",        10)
      << col("Throughput", 12)
      << col("Jain",       8)
      << col("Util%",      8)
      << col("P99-P50",    10)
      << "\n";

    f << std::string(94, '-') << "\n";

    for (const auto* rs : sorted) {
        double p99_p50 = rs->offline.p99_waiting_time - rs->offline.p50_waiting_time;
        f << col(rs->scheduler_name,               14)
          << col(rs->workload_name,                 12)
          << col(fmtDouble(rs->arrival_rate, 2),    8)
          << col(fmtDouble(rs->offline.avg_waiting_time), 10)
          << col(fmtDouble(rs->offline.p99_waiting_time), 10)
          << col(fmtDouble(rs->online.throughput),   12)
          << col(fmtDouble(rs->offline.jain_fairness_index), 8)
          << col(fmtDouble(rs->online.utilization * 100.0, 1), 8)
          << col(fmtDouble(p99_p50),                10)
          << "\n";
    }

    f << "\nTotal runs: " << summaries.size() << "\n";
}

void SummaryWriter::writeJSONSummary(const std::string&             path,
                                      const std::vector<RunSummary>& summaries) const {
    nlohmann::json j = nlohmann::json::array();

    for (const auto& rs : summaries) {
        nlohmann::json entry;
        entry["scheduler"]       = rs.scheduler_name;
        entry["workload"]        = rs.workload_name;
        entry["seed"]            = rs.seed;
        entry["arrival_rate"]    = rs.arrival_rate;
        entry["avg_waiting_time"]= rs.offline.avg_waiting_time;
        entry["p50"]             = rs.offline.p50_waiting_time;
        entry["p95"]             = rs.offline.p95_waiting_time;
        entry["p99"]             = rs.offline.p99_waiting_time;
        entry["throughput"]      = rs.online.throughput;
        entry["utilization"]     = rs.online.utilization;
        entry["jain"]            = rs.offline.jain_fairness_index;
        entry["lyapunov_v"]      = rs.online.lyapunov_v;
        entry["lyapunov_drift"]  = rs.online.lyapunov_drift;
        entry["max_starvation"]  = rs.offline.max_starvation_ticks;
        entry["queue_mean"]      = rs.offline.queue_stats.mean;
        entry["queue_max"]       = rs.offline.queue_stats.max;
        entry["oscillation"]     = rs.offline.oscillation_frequency;
        entry["steady_state"]    = rs.offline.time_to_steady_state;
        entry["context_switches"]= rs.offline.context_switch_rate;
        j.push_back(entry);
    }

    std::ofstream f(path);
    if (!f.is_open()) {
        throw std::runtime_error("SummaryWriter: cannot write '" + path + "'");
    }
    f << j.dump(2);
}

} // namespace embi
