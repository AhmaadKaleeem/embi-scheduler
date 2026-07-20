/**
 * @file Simulator.cpp
 * @brief Simulator implementation: dependency injection and run orchestration.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#include "core/Simulator.hpp"

#include "core/EventLoop.hpp"
#include "schedulers/CmuScheduler.hpp"
#include "schedulers/EMBIScheduler.hpp"
#include "schedulers/FCFSScheduler.hpp"
#include "schedulers/MaxWeightScheduler.hpp"
#include "schedulers/RoundRobinScheduler.hpp"
#include "workloads/BurstyWorkload.hpp"
#include "workloads/HeavyTailWorkload.hpp"
#include "workloads/LockContentionWorkload.hpp"
#include "workloads/PoissonWorkload.hpp"
#include "workloads/TraceLoader.hpp"
#include "workloads/UniformWorkload.hpp"
#include "workloads/WorkloadProfile.hpp"
#include "utils/FileUtils.hpp"

#include <stdexcept>

namespace embi {

// ─── Workload factory ────────────────────────────────────────────────────────

std::unique_ptr<BaseWorkload> Simulator::buildWorkload(const Config& config) {
    // Profile overrides workload_name
    if (config.workload_profile.has_value()) {
        return WorkloadProfile::build(config.workload_profile.value(), config.seed);
    }

    const std::string& name = config.workload_name;

    if (name == "uniform") {
        return std::make_unique<UniformWorkload>(
            config.seed, config.uniform_lo, config.uniform_hi);
    }
    if (name == "poisson") {
        return std::make_unique<PoissonWorkload>(config.seed, config.arrival_rate);
    }
    if (name == "bursty") {
        return std::make_unique<BurstyWorkload>(
            config.seed,
            config.burst_on_rate,
            config.burst_off_rate,
            config.burst_p_on_off,
            config.burst_p_off_on);
    }
    if (name == "heavy_tail") {
        return std::make_unique<HeavyTailWorkload>(
            config.seed, config.pareto_scale, config.pareto_shape);
    }
    if (name == "trace") {
        if (!config.trace_file.has_value()) {
            throw std::invalid_argument(
                "Simulator: workload 'trace' requires trace_file in config");
        }
        return std::make_unique<TraceLoader>(config.trace_file.value(), /*looping=*/true);
    }
    if (name == "lock_contention") {
        return std::make_unique<LockContentionWorkload>(
            config.seed,
            config.num_locks,
            config.num_processes,
            config.lock_request_rate,
            config.lock_hold_mean);
    }

    throw std::invalid_argument(
        "Simulator::buildWorkload: unknown workload '" + name + "'");
}

// ─── Scheduler factory ────────────────────────────────────────────────────────

std::unique_ptr<BaseScheduler> Simulator::buildScheduler(const Config& config) {
    const std::string& name = config.scheduler_name;

    if (name == "embi") {
        return std::make_unique<EMBIScheduler>(config, /*clip=*/true);
    }
    if (name == "embi_unclipped") {
        return std::make_unique<EMBIScheduler>(config, /*clip=*/false);
    }
    if (name == "maxweight") {
        return std::make_unique<MaxWeightScheduler>(config);
    }
    if (name == "cmu") {
        return std::make_unique<CmuScheduler>(config);
    }
    if (name == "rr") {
        return std::make_unique<RoundRobinScheduler>(config);
    }
    if (name == "fcfs") {
        return std::make_unique<FCFSScheduler>(config);
    }

    throw std::invalid_argument(
        "Simulator::buildScheduler: unknown scheduler '" + name + "'");
}

// ─── Construction ────────────────────────────────────────────────────────────

Simulator::Simulator(const Config& config)
    : config_(config)
    , workload_(buildWorkload(config))
    , scheduler_(buildScheduler(config))
    , online_metrics_(std::make_unique<OnlineMetrics>(config.num_processes, 1000))
    , offline_metrics_(std::make_unique<OfflineMetrics>(config.num_processes, config.ticks))
    , stats_db_(StatisticsDatabase::create(config))
{}

Simulator::Simulator(const Config&                     config,
                     std::unique_ptr<BaseWorkload>      workload,
                     std::unique_ptr<BaseScheduler>     scheduler,
                     std::unique_ptr<StatisticsDatabase> stats_db)
    : config_(config)
    , workload_(std::move(workload))
    , scheduler_(std::move(scheduler))
    , online_metrics_(std::make_unique<OnlineMetrics>(config.num_processes, 1000))
    , offline_metrics_(std::make_unique<OfflineMetrics>(config.num_processes, config.ticks))
    , stats_db_(std::move(stats_db))
{}

// ─── Run ─────────────────────────────────────────────────────────────────────

Results Simulator::run() {
    // Run the event loop
    EventLoop loop(config_,
                   workload_.get(),
                   scheduler_.get(),
                   online_metrics_.get(),
                   offline_metrics_.get(),
                   *stats_db_);
    loop.run();

    // Collect the final process state
    const auto& procs = loop.processes();

    // Compute offline report
    OfflineReport offline = offline_metrics_->compute(procs, config_.ticks);

    // Get online snapshot
    OnlineSnapshot online = online_metrics_->snapshot();

    // Write summary files (if not null logger)
    if (!config_.null_log) {
        FileUtils::ensureDirectory(config_.output_dir);

        std::string summary_txt = FileUtils::join(config_.output_dir, "summary.txt");
        std::string summary_json = FileUtils::join(config_.output_dir, "summary.json");

        stats_db_->writeSummaryTxt(summary_txt, online, offline);
        stats_db_->exportJSONSummary(summary_json, online, offline);
    }

    // Close the logger
    stats_db_->close();

    // Build Results
    Results results;
    results.scheduler_name = config_.scheduler_name;
    results.workload_name  = config_.workload_name;
    results.seed           = config_.seed;
    results.arrival_rate   = config_.arrival_rate;
    results.online         = online;
    results.offline        = offline;

    return results;
}

std::string_view Simulator::schedulerName() const noexcept {
    return scheduler_->name();
}

std::string_view Simulator::workloadName() const noexcept {
    return workload_->name();
}

} // namespace embi
