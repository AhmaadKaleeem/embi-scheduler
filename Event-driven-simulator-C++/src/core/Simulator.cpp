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
#include "schedulers/EMBIAblatedScheduler.hpp"
#include "schedulers/SJFScheduler.hpp"
#include "schedulers/CFSScheduler.hpp"
#include "schedulers/GSQScheduler.hpp"
#include "schedulers/FCFSScheduler.hpp"
#include "schedulers/MaxWeightScheduler.hpp"
#include "schedulers/RoundRobinScheduler.hpp"
#include "schedulers/HybridEMBIScheduler.hpp"
#include "workloads/BurstyWorkload.hpp"
#include "workloads/HeavyTailWorkload.hpp"
#include "workloads/LockContentionWorkload.hpp"
#include "workloads/PoissonWorkload.hpp"
#include "workloads/TraceLoader.hpp"
#include "workloads/UniformWorkload.hpp"
#include "workloads/WorkloadProfile.hpp"
#include "utils/FileUtils.hpp"

#include "events/SyntheticEventSource.hpp"
#include "events/TraceReplaySource.hpp"
#include "trace/AlibabaParser.hpp"
#include "trace/TraceValidator.hpp"
#include "trace/TraceAnalyzer.hpp"
#include "trace/TraceStore.hpp"
#include "trace/ReplayEngine.hpp"
#include "trace/TraceConfig.hpp"

namespace embi {

// ─── Workload & EventSource factory ──────────────────────────────────────────

std::unique_ptr<IEventSource> Simulator::buildEventSource(const Config& config) {
    std::unique_ptr<BaseWorkload> wl;

    // Profile overrides workload_name
    if (config.workload_profile.has_value()) {
        wl = WorkloadProfile::build(config.workload_profile.value(), config.seed);
    } else {
        const std::string& name = config.workload_name;

        if (name == "uniform") {
            wl = std::make_unique<UniformWorkload>(
                config.seed, config.uniform_lo, config.uniform_hi);
        } else if (name == "poisson") {
            wl = std::make_unique<PoissonWorkload>(config.seed, config.arrival_rate);
        } else if (name == "bursty") {
            wl = std::make_unique<BurstyWorkload>(
                config.seed,
                config.burst_on_rate,
                config.burst_off_rate,
                config.burst_p_on_off,
                config.burst_p_off_on);
        } else if (name == "heavy_tail") {
            wl = std::make_unique<HeavyTailWorkload>(
                config.seed, config.pareto_scale, config.pareto_shape);
        } else if (name == "trace") {
            // Load and parse the trace
            AlibabaParser parser;
            auto records = parser.parse(config.trace_file.value());
            
            // Validate the trace
            TraceValidator::validate(records, false); // false = warn only for now
            
            // Store the trace
            auto store = std::make_shared<TraceStore>(std::move(records));
            
            // Create replay engine
            TraceConfig trace_cfg;
            trace_cfg.trace_file = config.trace_file;
            trace_cfg.format = "alibaba";
            trace_cfg.num_processes = config.num_processes;
            auto engine = std::make_unique<ReplayEngine>(store, trace_cfg);
            
            return std::make_unique<TraceReplaySource>(std::move(engine));
        } else if (name == "lock_contention") {
            wl = std::make_unique<LockContentionWorkload>(
                config.seed, config.num_locks, config.num_processes, config.lock_request_rate, config.lock_hold_mean);
        } else {
            throw std::invalid_argument("Unknown workload type: " + name);
        }
    }

    return std::make_unique<SyntheticEventSource>(config, std::move(wl));
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
    if (name == "hybrid_embi") {
        return std::make_unique<HybridEMBIScheduler>(config);
    }
    if (name == "embi_ablated") {
        return std::make_unique<EMBIAblatedScheduler>(config);
    }
    if (name == "sjf") {
        return std::make_unique<SJFScheduler>(config);
    }
    if (name == "cfs") {
        return std::make_unique<CFSScheduler>(config);
    }
    if (name == "gsq") {
        return std::make_unique<GSQScheduler>();
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
    , event_source_(buildEventSource(config))
    , scheduler_(buildScheduler(config))
    , online_metrics_(std::make_unique<OnlineMetrics>(config.num_processes, 1000))
    , offline_metrics_(std::make_unique<OfflineMetrics>(config.num_processes, config.ticks))
    , stats_db_(StatisticsDatabase::create(config))
{}

Simulator::Simulator(const Config&                       config,
                     std::unique_ptr<IEventSource>       event_source,
                     std::unique_ptr<BaseScheduler>      scheduler,
                     std::unique_ptr<StatisticsDatabase> stats_db)
    : config_(config)
    , event_source_(std::move(event_source))
    , scheduler_(std::move(scheduler))
    , online_metrics_(std::make_unique<OnlineMetrics>(config.num_processes, 1000))
    , offline_metrics_(std::make_unique<OfflineMetrics>(config.num_processes, config.ticks))
    , stats_db_(std::move(stats_db))
{}

// ─── Run ─────────────────────────────────────────────────────────────────────

Results Simulator::run() {
    // Run the event loop
    EventLoop loop(config_,
                   event_source_.get(),
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

    // Write summary files (always write aggregate results)
    FileUtils::ensureDirectory(config_.output_dir);

    std::string summary_txt = FileUtils::join(config_.output_dir, "summary.txt");
    std::string summary_json = FileUtils::join(config_.output_dir, "summary.json");
    std::string v_trace_csv = FileUtils::join(config_.output_dir, "v_trace.csv");
    std::string manifest_json = FileUtils::join(config_.output_dir, "artifact_manifest.json");
    std::string hybrid_trace_csv = FileUtils::join(config_.output_dir, "hybrid_trace.csv");

    stats_db_->writeSummaryTxt(summary_txt, online, offline);
    stats_db_->exportJSONSummary(summary_json, online, offline);
    stats_db_->exportLyapunovTrace(v_trace_csv, offline, offline.v_samples);
    stats_db_->exportManifest(manifest_json);
    stats_db_->exportHybridTrace(hybrid_trace_csv, offline);

    // Close the logger
    stats_db_->close();

    // Build Results
    Results results;
    results.scheduler_name = config_.scheduler_name;
    results.workload_name  = config_.workload_name;
    results.seed           = config_.seed;
    results.arrival_rate   = config_.arrival_rate;
    results.arrival_rate_asymmetric = config_.arrival_rate_asymmetric;
    results.config         = config_;
    results.online         = online;
    results.offline        = offline;

    return results;
}

std::string_view Simulator::schedulerName() const noexcept {
    return scheduler_->name();
}

std::string_view Simulator::workloadName() const noexcept {
    return config_.workload_name;
}

} // namespace embi
