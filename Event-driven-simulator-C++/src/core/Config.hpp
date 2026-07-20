/**
 * @file Config.hpp
 * @brief Simulation configuration: all parameters, validation, and experiment sweeps.
 *
 * Config is the single source of truth for every simulation run. It is
 * constructed once, validated immediately, and then passed by const reference
 * throughout the entire call stack — there is no global mutable state.
 *
 * ExperimentConfig extends Config to describe multi-scheduler / multi-seed
 * parameter sweeps.  Its expand() method produces a flat list of Config
 * instances — one per combination — ready to be handed to Experiment.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#pragma once

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace embi {

// ─── Valid scheduler names ────────────────────────────────────────────────────

/// Compile-time list of all supported scheduler identifiers.
inline constexpr const char* kValidSchedulers[] = {
    "embi", "embi_unclipped", "maxweight", "cmu", "rr", "fcfs"};

/// Compile-time list of all supported workload identifiers.
inline constexpr const char* kValidWorkloads[] = {
    "uniform", "poisson", "bursty", "heavy_tail", "trace"};

/// Compile-time list of all supported workload profile names.
inline constexpr const char* kValidProfiles[] = {
    "web_server", "database", "cloud_vm", "embedded", "real_time",
    "interactive_desktop"};

// ─── Config ──────────────────────────────────────────────────────────────────

/**
 * @class Config
 * @brief Immutable simulation configuration validated at construction.
 *
 * All fields have sensible defaults. Call validate() to throw on bad values.
 * ConfigLoader::load() and CLI::parse() both produce validated Config objects.
 *
 * @par Example
 * @code
 * embi::Config cfg;
 * cfg.scheduler_name  = "embi";
 * cfg.workload_name   = "poisson";
 * cfg.ticks           = 500'000;
 * cfg.num_processes   = 32;
 * cfg.seed            = 42;
 * cfg.M               = 8.0;
 * cfg.validate();
 * @endcode
 */
struct Config {
    // ── Simulation ────────────────────────────────────────────────────────────
    uint64_t    ticks{1'000'000};   ///< Number of simulation ticks.
    std::size_t num_processes{16};  ///< Number of concurrent processes.
    uint64_t    seed{42};           ///< PRNG seed for reproducibility.

    // ── Scheduling ────────────────────────────────────────────────────────────
    std::string scheduler_name{"embi"};  ///< Scheduler identifier.
    bool        embi_clipped{true};      ///< If true, EMBI clips score at 0.

    // ── Workload ──────────────────────────────────────────────────────────────
    std::string                workload_name{"poisson"};  ///< Workload identifier.
    std::optional<std::string> workload_profile;           ///< Named preset (overrides workload_name).
    std::optional<std::string> trace_file;                 ///< Path to trace CSV (for workload="trace").

    // ── Rate parameters ───────────────────────────────────────────────────────
    double arrival_rate{0.5};   ///< Mean job arrivals per tick per process.
    std::vector<double> arrival_rate_asymmetric; ///< Optional per-process rates.
    double service_rate{1.0};   ///< Mean jobs completed per tick (≤ 1 for unit service).
    double alpha{0.1};          ///< EWMA smoothing factor for arrival rate estimates.
    double beta{0.1};           ///< EWMA smoothing factor for service rate estimates.
    double M{10.0};             ///< EMBI service-capacity bound (independent of N).

    // ── Workload-specific ─────────────────────────────────────────────────────
    double uniform_lo{0.5};        ///< U(lo, hi) inter-arrival lower bound (ticks).
    double uniform_hi{2.0};        ///< U(lo, hi) inter-arrival upper bound (ticks).
    double pareto_shape{1.5};      ///< Pareto shape α (heavy-tail exponent).
    double pareto_scale{1.0};      ///< Pareto minimum value x_m.
    double burst_on_rate{0.8};     ///< Bursty ON-state arrival rate.
    double burst_off_rate{0.05};   ///< Bursty OFF-state arrival rate.
    double burst_p_on_off{0.1};    ///< P(ON → OFF) per tick.
    double burst_p_off_on{0.3};    ///< P(OFF → ON) per tick.

    // ── Logging ───────────────────────────────────────────────────────────────
    std::string output_dir{"output"};  ///< Directory for output files.
    uint64_t    log_freq{1};           ///< Log every N ticks (1 = every tick).
    bool        binary_log{false};     ///< Use BinaryLogger instead of CSVLogger.
    bool        null_log{false};       ///< Use NullLogger (maximum-speed benchmarking).

    // ── Experiment mode ───────────────────────────────────────────────────────
    bool                       experiment_mode{false};  ///< Run a parameter sweep.
    std::optional<std::string> config_file;             ///< Path to JSON/YAML config.

    // ─── Validation ──────────────────────────────────────────────────────────

    /**
     * @brief Validates all fields; throws std::invalid_argument on bad values.
     *
     * Checked invariants include:
     *  - ticks > 0
     *  - num_processes ≥ 1
     *  - arrival_rate ∈ (0, 1]
     *  - service_rate > 0
     *  - alpha, beta ∈ (0, 1)
     *  - M > 0
     *  - scheduler_name ∈ kValidSchedulers
     *  - workload_name ∈ kValidWorkloads (unless workload_profile is set)
     *  - pareto_shape > 0, pareto_scale > 0
     *  - uniform_lo < uniform_hi
     *  - burst_* probabilities in (0, 1)
     *
     * @throws std::invalid_argument with a descriptive message.
     * @complexity O(1)
     */
    void validate() const;

    /**
     * @brief Returns a Config with all defaults — identical to value-initialization.
     * @return Default-constructed Config.
     * @complexity O(1)
     */
    [[nodiscard]] static Config defaults() noexcept;
};

// ─── ExperimentConfig ─────────────────────────────────────────────────────────

/**
 * @class ExperimentConfig
 * @brief Describes a multi-scheduler / multi-workload / multi-seed parameter sweep.
 *
 * ExperimentConfig::expand() generates one Config per
 * (scheduler × workload × seed × arrival_rate) combination, ready for
 * parallel execution by the Experiment manager.
 *
 * @par Example JSON
 * @code{.json}
 * {
 *   "schedulers":    ["embi", "maxweight", "rr"],
 *   "workloads":     ["poisson", "bursty"],
 *   "seeds":         [42, 123, 456],
 *   "arrival_rates": [0.3, 0.5, 0.8],
 *   "ticks":         1000000,
 *   "num_processes": 64,
 *   "M":             10.0
 * }
 * @endcode
 */
struct ExperimentConfig {
    // ── Sweep axes ────────────────────────────────────────────────────────────
    std::vector<std::string> schedulers{"embi"};      ///< Scheduler names to sweep.
    std::vector<std::string> workloads{"poisson"};    ///< Workload names to sweep.
    std::vector<uint64_t>    seeds{42};               ///< Seeds to sweep.
    std::vector<double>      arrival_rates{0.5};      ///< Arrival rates to sweep.
    std::vector<double>      alphas;                  ///< Alphas to sweep.

    // ── Fixed parameters ──────────────────────────────────────────────────────
    uint64_t    ticks{1'000'000};
    std::size_t num_processes{16};
    double      service_rate{1.0};
    double      alpha{0.1};
    double      beta{0.1};
    double      M{10.0};
    double      pareto_shape{1.5};
    double      pareto_scale{1.0};
    double      burst_on_rate{0.8};
    double      burst_off_rate{0.05};
    double      burst_p_on_off{0.1};
    double      burst_p_off_on{0.3};
    std::optional<std::string> trace_file;

    // ── Output ────────────────────────────────────────────────────────────────
    std::string output_dir{"results"};
    uint64_t    log_freq{100};
    bool        binary_log{false};
    bool        null_log{false};

    /**
     * @brief Expands the sweep axes into a flat list of Config objects.
     *
     * Generates configs in the order:
     *   scheduler → workload → seed → arrival_rate (innermost)
     *
     * The output_dir of each config is set to:
     *   output_dir / scheduler / workload / seed_<seed>_rate_<rate>
     *
     * @return Vector of validated Config objects.
     * @throws std::invalid_argument if any axis value is invalid.
     * @complexity O(|schedulers| × |workloads| × |seeds| × |arrival_rates|)
     */
    [[nodiscard]] std::vector<Config> expand() const;

    /**
     * @brief Total number of simulation runs this sweep will produce.
     * @return Number of (scheduler × workload × seed × rate) combinations.
     * @complexity O(1)
     */
    [[nodiscard]] std::size_t totalRuns() const noexcept;
};

} // namespace embi
