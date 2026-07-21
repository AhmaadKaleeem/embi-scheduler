/**
 * @file Experiment.cpp
 * @brief Experiment implementation: sequential sweep execution.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#include "core/Experiment.hpp"

#include "core/Simulator.hpp"
#include "utils/FileUtils.hpp"
#include "utils/Timer.hpp"

#include <chrono>
#include <iostream>
#include <stdexcept>

namespace embi {

Experiment::Experiment(const ExperimentConfig& exp_config)
    : exp_config_(exp_config)
    , configs_(exp_config.expand())
{
    results_.reserve(configs_.size());
}

void Experiment::run(ProgressCallback progress) {
    const std::size_t total = configs_.size();
    results_.clear();
    results_.reserve(total);

    std::cout << "[Experiment] Starting " << total << " simulation runs.\n";

    for (std::size_t i = 0; i < total; ++i) {
        const Config& cfg = configs_[i];

        std::cout << "[Run " << (i + 1) << "/" << total << "] "
                  << cfg.scheduler_name << " | "
                  << cfg.workload_name  << " | seed=" << cfg.seed
                  << " | rate=" << cfg.arrival_rate
                  << " | ticks=" << cfg.ticks << "\n";

        Timer run_timer;

        Simulator sim(cfg);
        Results result = sim.run();

        double elapsed_ms = run_timer.elapsed_ms();

        std::cout << "  ✓ Done in " << elapsed_ms << " ms | "
                  << "Jain=" << result.offline.jain_fairness_index
                  << " | AvgWait=" << result.offline.avg_waiting_time
                  << " | P99=" << result.offline.p99_waiting_time << "\n";

        results_.push_back(std::move(result));

        if (progress) {
            progress(i + 1, total, results_.back());
        }
    }

    // Write experiment-level summary
    writeSummary(exp_config_.output_dir);

    std::cout << "[Experiment] Complete. Results in '"
              << exp_config_.output_dir << "'\n";
}

void Experiment::writeSummary(const std::string& output_dir) const {
    if (results_.empty()) return;

    FileUtils::ensureDirectory(output_dir);

    // Build RunSummary vector for SummaryWriter
    std::vector<RunSummary> summaries;
    summaries.reserve(results_.size());

    for (const auto& r : results_) {
        RunSummary rs;
        rs.scheduler_name = r.scheduler_name;
        rs.workload_name  = r.workload_name;
        rs.seed           = r.seed;
        rs.arrival_rate   = r.arrival_rate;
        rs.arrival_rate_asymmetric = r.arrival_rate_asymmetric;
        rs.config         = r.config;
        rs.online         = r.online;
        rs.offline        = r.offline;
        summaries.push_back(std::move(rs));
    }

    std::string txt_path  = FileUtils::join(output_dir, "summary.txt");
    std::string json_path = FileUtils::join(output_dir, "summary.json");

    writer_.writeComparativeTable(txt_path, summaries);
    writer_.writeJSONSummary(json_path, summaries);

    std::cout << "[Experiment] Summary written to:\n"
              << "  " << txt_path  << "\n"
              << "  " << json_path << "\n";
}

const std::vector<Results>& Experiment::results() const noexcept {
    return results_;
}

std::size_t Experiment::totalRuns() const noexcept {
    return configs_.size();
}

} // namespace embi
