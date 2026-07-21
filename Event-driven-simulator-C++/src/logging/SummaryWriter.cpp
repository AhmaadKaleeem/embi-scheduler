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

std::string fmtRates(const std::vector<double>& rates) {
    if (rates.empty()) return "";

    std::ostringstream oss;
    for (std::size_t i = 0; i < rates.size(); ++i) {
        if (i > 0) oss << ",";
        oss << fmtDouble(rates[i], 2);
    }
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
      << col("Asym",       20)
      << col("AvgWait",    10)
      << col("P99",        10)
      << col("Throughput", 12)
      << col("Jain",       8)
      << col("Util%",      8)
      << col("P99-P50",    10)
      << "\n";

    f << std::string(114, '-') << "\n";

    for (const auto* rs : sorted) {
        double p99_p50 = rs->offline.p99_waiting_time - rs->offline.p50_waiting_time;
        f << col(rs->scheduler_name,               14)
          << col(rs->workload_name,                 12)
          << col(fmtDouble(rs->arrival_rate, 2),    8)
          << col(fmtRates(rs->arrival_rate_asymmetric), 20)
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
        entry["arrival_rate_asymmetric"] = rs.arrival_rate_asymmetric;
        
        // ── Config ────────────────────────────────────────────────────────────
        entry["config"] = {
            {"ticks", rs.config.ticks},
            {"warmup_ticks", rs.config.warmup_ticks},
            {"num_processes", rs.config.num_processes},
            {"M", rs.config.M},
            {"alpha", rs.config.alpha},
            {"beta", rs.config.beta},
            {"epsilon_total", rs.config.epsilon_total},
            {"tau_constant", rs.config.tau_constant_bound},
            {"clip_scores", rs.config.embi_clipped},
            {"context_switch_cost", rs.config.context_switch_cost},
            {"rng_stream", "independent_per_experiment"},
            {"rng_engine", "std::mt19937_64"},
            {"queue_capacity", "infinite"},
            {"floating_point", "double"},
            {"git_commit", rs.config.git_commit_hash},
            {"binary_hash", rs.config.binary_sha256},
            {"config_hash", rs.config.config_hash}
        };

        // ── Platform Info ─────────────────────────────────────────────────────
        entry["platform"] = {
#if defined(_WIN32) || defined(_WIN64)
            {"os", "Windows"},
#elif defined(__linux__)
            {"os", "Linux"},
#elif defined(__APPLE__)
            {"os", "macOS"},
#else
            {"os", "Unknown"},
#endif
#if defined(__clang__)
            {"compiler", "Clang"},
            {"compiler_version", __clang_version__},
#elif defined(__GNUC__)
            {"compiler", "GCC"},
            {"compiler_version", std::to_string(__GNUC__) + "." + std::to_string(__GNUC_MINOR__)},
#elif defined(_MSC_VER)
            {"compiler", "MSVC"},
            {"compiler_version", std::to_string(_MSC_FULL_VER)},
#else
            {"compiler", "Unknown"},
            {"compiler_version", "Unknown"},
#endif
#ifdef NDEBUG
            {"build_type", "Release (optimized)"}
#else
            {"build_type", "Debug"}
#endif
        };

        // ── Metrics ───────────────────────────────────────────────────────────
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
        entry["queue_median"]    = rs.offline.queue_stats.median;
        entry["queue_p95"]       = rs.offline.queue_stats.p95;
        entry["queue_p99"]       = rs.offline.queue_stats.p99;
        entry["queue_max"]       = rs.offline.queue_stats.max;
        entry["oscillation"]     = rs.offline.oscillation_frequency;
        entry["steady_state"]    = rs.offline.time_to_steady_state;
        entry["context_switches"]= rs.offline.context_switch_rate;
        entry["hybrid_fraction"] = rs.online.hybrid_embi_fraction;
        
        // ── Branch & Hybrid Stats ─────────────────────────────────────────────
        entry["hybrid_avg_streak"] = rs.offline.hybrid_avg_streak;
        entry["hybrid_max_streak"] = rs.offline.hybrid_max_streak;
        entry["hybrid_transitions"] = rs.offline.hybrid_transition_count;
        
        // ── Runtime Overhead ──────────────────────────────────────────────────
        entry["avg_scheduler_runtime_ns"] = rs.offline.avg_scheduler_runtime_ns;
        entry["max_scheduler_runtime_ns"] = rs.offline.max_scheduler_runtime_ns;
        
        // ── Score Components ──────────────────────────────────────────────────
        double sum_components = rs.offline.avg_queue_term + rs.offline.avg_prediction_term + rs.offline.avg_penalty_term + 1e-9;
        entry["components"] = {
            {"avg_queue", rs.offline.avg_queue_term},
            {"avg_prediction", rs.offline.avg_prediction_term},
            {"avg_penalty", rs.offline.avg_penalty_term},
            {"pct_queue", (rs.offline.avg_queue_term / sum_components) * 100.0},
            {"pct_prediction", (rs.offline.avg_prediction_term / sum_components) * 100.0},
            {"pct_penalty", (rs.offline.avg_penalty_term / sum_components) * 100.0}
        };

        j.push_back(entry);
    }

    std::ofstream f(path);
    if (!f.is_open()) {
        throw std::runtime_error("SummaryWriter: cannot write '" + path + "'");
    }
    f << j.dump(2);
}

} // namespace embi
