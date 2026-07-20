/**
 * @file Experiment.hpp
 * @brief Parameter-sweep experiment manager: runs multiple Simulations in sequence.
 *
 * Experiment expands an ExperimentConfig into N individual Configs,
 * runs each sequentially (or in parallel — future extension), and
 * aggregates results for SummaryWriter.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#pragma once

#include "core/Config.hpp"
#include "core/Results.hpp"
#include "logging/SummaryWriter.hpp"

#include <functional>
#include <string>
#include <vector>

namespace embi {

/**
 * @class Experiment
 * @brief Runs a parameter sweep and aggregates results.
 *
 * @par Example
 * @code
 * embi::ExperimentConfig exp;
 * exp.schedulers    = {"embi", "maxweight", "rr"};
 * exp.workloads     = {"poisson", "bursty"};
 * exp.seeds         = {42, 123, 456};
 * exp.arrival_rates = {0.3, 0.5, 0.8};
 * exp.ticks         = 500'000;
 *
 * embi::Experiment experiment(exp);
 * experiment.run();
 * @endcode
 */
class Experiment {
public:
    /**
     * @brief Constructs an Experiment from an ExperimentConfig.
     * @param exp_config  Sweep configuration describing all axes.
     */
    explicit Experiment(const ExperimentConfig& exp_config);

    /**
     * @brief Callback type for progress notifications.
     * @param completed  Number of completed runs.
     * @param total      Total number of runs.
     * @param result     Reference to the most recently completed result.
     */
    using ProgressCallback = std::function<void(std::size_t completed,
                                                  std::size_t total,
                                                  const Results& result)>;

    // ─── Primary interface ─────────────────────────────────────────────────────

    /**
     * @brief Runs all simulation configurations sequentially.
     *
     * Progress is reported to the callback (if set) after each run completes.
     * Results are stored internally and can be retrieved via results().
     *
     * @param progress  Optional progress callback.
     * @complexity O(N_runs × ticks × n_processes)
     */
    void run(ProgressCallback progress = nullptr);

    /**
     * @brief Writes a comparative summary table and JSON file.
     *
     * Called automatically at the end of run(), but can also be called
     * manually (e.g., to regenerate after post-processing).
     *
     * @param output_dir  Directory to write summary.txt and summary.json.
     * @throws std::runtime_error if files cannot be written.
     */
    void writeSummary(const std::string& output_dir) const;

    /**
     * @brief Returns all results after run() completes.
     * @return Const reference to the vector of Results, one per configuration.
     * @complexity O(1)
     */
    [[nodiscard]] const std::vector<Results>& results() const noexcept;

    /**
     * @brief Returns the total number of runs the sweep will perform.
     * @complexity O(1)
     */
    [[nodiscard]] std::size_t totalRuns() const noexcept;

private:
    ExperimentConfig       exp_config_;
    std::vector<Config>    configs_;
    std::vector<Results>   results_;
    SummaryWriter          writer_;
};

} // namespace embi
