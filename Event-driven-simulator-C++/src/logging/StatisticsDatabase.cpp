/**
 * @file StatisticsDatabase.cpp
 * @brief StatisticsDatabase implementation.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#include "logging/StatisticsDatabase.hpp"

#include "logging/BinaryLogger.hpp"
#include "logging/CSVLogger.hpp"
#include "logging/NullLogger.hpp"
#include "utils/FileUtils.hpp"

#include <nlohmann/json.hpp>

#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace embi {

// ─── Factory ─────────────────────────────────────────────────────────────────

std::unique_ptr<StatisticsDatabase>
StatisticsDatabase::create(const Config& config) {
    if (!config.null_log) {
        FileUtils::ensureDirectory(config.output_dir);
    }

    std::unique_ptr<Logger> logger;

    if (config.null_log) {
        logger = std::make_unique<NullLogger>();
    } else if (config.binary_log) {
        std::string path = FileUtils::join(config.output_dir, "run.bin");
        logger = std::make_unique<BinaryLogger>(path);
    } else {
        std::string path = FileUtils::join(config.output_dir, "run.csv");
        logger = std::make_unique<CSVLogger>(path);
    }

    return std::make_unique<StatisticsDatabase>(std::move(logger), config);
}

// ─── Construction ────────────────────────────────────────────────────────────

StatisticsDatabase::StatisticsDatabase(std::unique_ptr<Logger> logger,
                                        const Config&           config)
    : logger_(std::move(logger))
    , config_(config)
{}

// ─── Core output ─────────────────────────────────────────────────────────────

void StatisticsDatabase::record(const LogRecord& record) {
    logger_->write(record);
}

void StatisticsDatabase::flush() {
    logger_->flush();
}

void StatisticsDatabase::close() {
    logger_->close();
}

// ─── Statistics ──────────────────────────────────────────────────────────────

void StatisticsDatabase::recordStat(const std::string& name, double value) {
    stats_.record(name, value);
}

const Statistics& StatisticsDatabase::stats() const noexcept {
    return stats_;
}

// ─── Summary text ─────────────────────────────────────────────────────────────

void StatisticsDatabase::writeSummaryTxt(const std::string&    path,
                                          const OnlineSnapshot& online,
                                          const OfflineReport&  r) const {
    std::ofstream f(path);
    if (!f.is_open()) {
        throw std::runtime_error("StatisticsDatabase: cannot write '" + path + "'");
    }

    f << "╔══════════════════════════════════════════════════════════╗\n"
      << "║         EMBI Scheduling Simulator - Run Summary          ║\n"
      << "╚══════════════════════════════════════════════════════════╝\n\n";

    f << "── Simulation Parameters ────────────────────────────────────\n"
      << "  Scheduler    : " << config_.scheduler_name   << "\n"
      << "  Workload     : " << config_.workload_name    << "\n"
      << "  Ticks        : " << config_.ticks            << "\n"
      << "  Processes    : " << config_.num_processes     << "\n"
      << "  Seed         : " << config_.seed             << "\n"
      << "  Arrival rate : " << config_.arrival_rate     << "\n"
      << "  Service rate : " << config_.service_rate     << "\n"
      << "  Alpha        : " << config_.alpha            << "\n"
      << "  Beta         : " << config_.beta             << "\n"
      << "  M            : " << config_.M                << "\n\n";

    f << std::fixed << std::setprecision(6);

    f << "── Online Metrics (final tick) ──────────────────────────────\n"
      << "  Lyapunov V(t)    : " << online.lyapunov_v     << "\n"
      << "  Lyapunov Drift   : " << online.lyapunov_drift  << "\n"
      << "  Rolling Throughput: " << online.throughput     << " jobs/tick\n"
      << "  CPU Utilization  : " << online.utilization * 100.0 << " %\n"
      << "  Completed Jobs   : " << online.completed_jobs  << "\n\n";

    f << "── Latency ──────────────────────────────────────────────────\n"
      << "  Avg Waiting Time  : " << r.avg_waiting_time   << " ticks\n"
      << "  P50 Waiting Time  : " << r.p50_waiting_time   << " ticks\n"
      << "  P95 Waiting Time  : " << r.p95_waiting_time   << " ticks\n"
      << "  P99 Waiting Time  : " << r.p99_waiting_time   << " ticks\n"
      << "  Max Waiting Time  : " << r.max_waiting_time   << " ticks\n"
      << "  Avg Turnaround    : " << r.avg_turnaround_time << " ticks\n\n";

    f << "── Stability ────────────────────────────────────────────────────────\n"
      << "  Avg Lyapunov V(t) : " << r.avg_lyapunov_v       << "\n"
      << "  Max Lyapunov V(t) : " << r.max_lyapunov_v       << "\n"
      << "  Oscillation Freq  : " << r.oscillation_frequency << " / 1000 ticks\n\n";

    f << "── Fairness ─────────────────────────────────────────────────────────\n"
      << "  Jain Fairness Index  : " << r.jain_fairness_index     << "\n"
      << "  Max Starvation       : " << r.max_starvation_ticks    << " ticks\n"
      << "  Avg Starvation       : " << r.avg_starvation_ticks    << " ticks\n"
      << "  Starvation Events    : " << r.total_starvation_events  << "\n\n";

    f << "── Queue Statistics ──────────────────────────────────────────\n"
      << "  Mean Queue Length  : " << r.queue_stats.mean     << "\n"
      << "  Max  Queue Length  : " << r.queue_stats.max      << "\n"
      << "  Min  Queue Length  : " << r.queue_stats.min      << "\n"
      << "  Median Queue Length: " << r.queue_stats.median   << "\n"
      << "  Queue Variance     : " << r.queue_stats.variance << "\n"
      << "  Queue Std Dev      : " << r.queue_stats.std_dev  << "\n\n";

    f << "── Stability ────────────────────────────────────────────────\n"
      << "  Time to Steady State  : "
      << (r.time_to_steady_state < 0.0 ? "not reached" : std::to_string(r.time_to_steady_state)) << "\n"
      << "  Oscillation Frequency : " << r.oscillation_frequency << " crossings/1000 ticks\n\n";

    f << "── Scheduler Diagnostics ────────────────────────────────────\n"
      << "  Avg Decision Entropy: " << r.avg_decision_entropy << " nats\n"
      << "  Avg Score Variance  : " << r.avg_score_variance   << "\n"
      << "  Context Switch Rate : " << r.context_switch_rate  * 100.0 << " %\n"
      << "  Total Throughput    : " << r.total_throughput     << " jobs/tick\n\n";

    f << "── Logger ───────────────────────────────────────────────────\n"
      << "  Logger type : " << logger_->typeName() << "\n"
      << "  Output dir  : " << config_.output_dir   << "\n";
}

// ─── JSON Summary ─────────────────────────────────────────────────────────────

void StatisticsDatabase::exportJSONSummary(const std::string&    path,
                                            const OnlineSnapshot& online,
                                            const OfflineReport&  r) const {
    nlohmann::json j;

    // Config
    j["config"]["scheduler"]    = config_.scheduler_name;
    j["config"]["workload"]     = config_.workload_name;
    j["config"]["ticks"]        = config_.ticks;
    j["config"]["num_processes"]= config_.num_processes;
    j["config"]["seed"]         = config_.seed;
    j["config"]["arrival_rate"] = config_.arrival_rate;
    j["config"]["service_rate"] = config_.service_rate;
    j["config"]["alpha"]        = config_.alpha;
    j["config"]["beta"]         = config_.beta;
    j["config"]["M"]            = config_.M;

    // Online
    j["online"]["lyapunov_v"]     = online.lyapunov_v;
    j["online"]["lyapunov_drift"] = online.lyapunov_drift;
    j["online"]["throughput"]     = online.throughput;
    j["online"]["utilization"]    = online.utilization;
    j["online"]["completed_jobs"] = online.completed_jobs;

    // Latency
    j["latency"]["avg_waiting_time"]  = r.avg_waiting_time;
    j["latency"]["p50_waiting_time"]  = r.p50_waiting_time;
    j["latency"]["p95_waiting_time"]  = r.p95_waiting_time;
    j["latency"]["p99_waiting_time"]  = r.p99_waiting_time;
    j["latency"]["max_waiting_time"]  = r.max_waiting_time;
    j["latency"]["avg_turnaround"]    = r.avg_turnaround_time;

    // Stability
    j["stability"]["avg_lyapunov_v"]        = r.avg_lyapunov_v;
    j["stability"]["max_lyapunov_v"]        = r.max_lyapunov_v;
    j["stability"]["time_to_steady_state"]  = r.time_to_steady_state;
    j["stability"]["oscillation_frequency"] = r.oscillation_frequency;
    j["stability"]["recovery_time"]         = r.recovery_time_after_burst;

    // Fairness
    j["fairness"]["jain_index"]        = r.jain_fairness_index;
    j["fairness"]["max_starvation"]    = r.max_starvation_ticks;
    j["fairness"]["starvation_events"] = r.total_starvation_events;

    // Queue
    j["queue"]["mean"]     = r.queue_stats.mean;
    j["queue"]["max"]      = r.queue_stats.max;
    j["queue"]["min"]      = r.queue_stats.min;
    j["queue"]["median"]   = r.queue_stats.median;
    j["queue"]["variance"] = r.queue_stats.variance;
    j["queue"]["std_dev"]  = r.queue_stats.std_dev;

    // Stability
    j["stability"]["time_to_steady_state"]  = r.time_to_steady_state;
    j["stability"]["oscillation_frequency"] = r.oscillation_frequency;

    // Scheduler
    j["scheduler_diag"]["avg_entropy"]        = r.avg_decision_entropy;
    j["scheduler_diag"]["avg_score_variance"] = r.avg_score_variance;
    j["scheduler_diag"]["context_switch_rate"]= r.context_switch_rate;
    j["scheduler_diag"]["total_throughput"]   = r.total_throughput;
    j["scheduler_diag"]["hybrid_embi_ratio"]  = r.hybrid_embi_mode_ratio;
    j["scheduler_diag"]["hybrid_mw_ratio"]    = r.hybrid_mw_mode_ratio;
    j["scheduler_diag"]["avg_tau"]            = r.avg_tau;
    j["scheduler_diag"]["avg_gap"]            = r.avg_gap;
    j["scheduler_diag"]["avg_eta_c"]          = r.avg_eta_c;

    // Aggregate statistics (from multi-seed runs if available)
    for (const auto& metric : stats_.metricNames()) {
        auto [mean, std_dev] = stats_.meanStdDev(metric);
        j["aggregate"][metric]["mean"]    = mean;
        j["aggregate"][metric]["std_dev"] = std_dev;
    }

    std::ofstream f(path);
    if (!f.is_open()) {
        throw std::runtime_error("StatisticsDatabase: cannot write '" + path + "'");
    }
    f << j.dump(2);
}

std::string StatisticsDatabase::loggerType() const noexcept {
    return logger_->typeName();
}

// ─── V(t) Trace ────────────────────────────────────────────────────────────

void StatisticsDatabase::exportLyapunovTrace(const std::string& path,
                                             const OfflineReport& /*r*/,
                                             const std::vector<double>& v_samples) const {
    std::ofstream f(path);
    if (!f.is_open()) return;

    f << "sample_index,tick_approx,v\n";
    std::size_t i = 0;
    for (double v : v_samples) {
        f << i++ << ",0," << v << "\n";
    }
}

void StatisticsDatabase::exportHybridTrace(const std::string& path,
                                           const OfflineReport& offline_report) const {
    if (offline_report.hybrid_samples.empty()) return;
    
    std::ofstream f(path);
    if (!f.is_open()) return;
    
    f << "tick,tau,gap,mode\n";
    for (const auto& sample : offline_report.hybrid_samples) {
        f << sample.tick << "," << sample.tau << "," << sample.gap << "," << sample.mode << "\n";
    }
}

void StatisticsDatabase::exportManifest(const std::string& path) const {
    nlohmann::json j;
    j["git_commit"]    = "development"; // can be populated via CMake in the future
    j["compiler"]      = "C++17";
    j["os"]            = "Windows";
    j["seed"]          = config_.seed;
    j["scheduler"]     = config_.scheduler_name;
    j["workload"]      = config_.workload_name;
    j["paper_version"] = "v1";
    
    // basic config info
    j["config"]["ticks"] = config_.ticks;
    j["config"]["num_processes"] = config_.num_processes;
    j["config"]["epsilon_total"] = config_.epsilon_total;
    
    std::ofstream f(path);
    if (!f.is_open()) return;
    f << j.dump(2);
}

} // namespace embi
