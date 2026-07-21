/**
 * @file Simulator.hpp
 * @brief High-level simulation driver: owns all components and runs one experiment.
 *
 * Simulator wires together the workload, scheduler, metrics, logging, and
 * EventLoop from a single Config. Call run() to execute and get Results.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#pragma once

#include "core/Config.hpp"
#include "core/OfflineMetrics.hpp"
#include "core/OnlineMetrics.hpp"
#include "core/Results.hpp"
#include "logging/StatisticsDatabase.hpp"
#include "schedulers/BaseScheduler.hpp"
#include "events/IEventSource.hpp"
#include <memory>

namespace embi {

/**
 * @class Simulator
 * @brief Owns all simulation components for a single Config and runs it.
 *
 * @par Example
 * @code
 * embi::Config cfg;
 * cfg.scheduler_name = "embi";
 * cfg.validate();
 *
 * embi::Simulator sim(cfg);
 * auto results = sim.run();
 * @endcode
 */
class Simulator {
public:
    /**
     * @brief Constructs a Simulator from a validated Config.
     *
     * Instantiates the workload, scheduler, metrics, and logger based on
     * the config fields. Does NOT run the simulation yet.
     *
     * @param config  Validated simulation configuration.
     * @throws std::invalid_argument if config has unknown scheduler/workload.
     * @throws std::runtime_error    if output files cannot be created.
     */
    explicit Simulator(const Config& config);

    /**
     * @brief Constructs a Simulator with explicitly injected dependencies.
     *
     * Use this constructor for testing — allows mocking the workload and
     * scheduler without touching filesystem or logger.
     *
     * @param config    Full simulation parameters.
     * @param event_source Hand-crafted event source generator.
     * @param scheduler Hand-crafted scheduler.
     * @param stats_db  Hand-crafted logger instance.
     * @complexity O(1)
     */
    Simulator(const Config&                       config,
              std::unique_ptr<IEventSource>       event_source,
              std::unique_ptr<BaseScheduler>      scheduler,
              std::unique_ptr<StatisticsDatabase> stats_db);

    // ─── Primary interface ────────────────────────────────────────────────────

    /**
     * @brief Runs the simulation and returns the results.
     *
     * Delegates to EventLoop::run(), then computes offline metrics and
     * writes summary files.
     *
     * @return Results object with populated online snapshot and offline report.
     * @complexity O(ticks × N)
     */
    Results run();

    /**
     * @brief Returns the scheduler name (for display / experiment logs).
     * @complexity O(1)
     */
    [[nodiscard]] std::string_view schedulerName() const noexcept;

    /**
     * @brief Returns the workload name.
     * @complexity O(1)
     */
    [[nodiscard]] std::string_view workloadName() const noexcept;

private:
    Config                             config_;
    std::unique_ptr<IEventSource>      event_source_;
    std::unique_ptr<BaseScheduler>     scheduler_;
    std::unique_ptr<OnlineMetrics>     online_metrics_;
    std::unique_ptr<OfflineMetrics>    offline_metrics_;
    std::unique_ptr<StatisticsDatabase> stats_db_;

    /// Builds the event source from config
    [[nodiscard]] static std::unique_ptr<IEventSource>
    buildEventSource(const Config& config);

    /// Builds the scheduler from config.scheduler_name.
    [[nodiscard]] static std::unique_ptr<BaseScheduler>
    buildScheduler(const Config& config);
};

} // namespace embi
