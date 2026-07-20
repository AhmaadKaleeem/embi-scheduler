/**
 * @file CLI.hpp
 * @brief Command-line interface parser for the EMBI simulator.
 *
 * Parses argv into a Config (single run) or an ExperimentConfig (sweep).
 * Uses only the C++ standard library — no external CLI parsing library.
 *
 * @par Usage example
 * @code{.sh}
 * ./embi_sim --scheduler embi --workload poisson --ticks 1000000 \
 *            --num-processes 64 --arrival-rate 0.5 --seed 42 \
 *            --M 10.0 --alpha 0.1 --beta 0.1 --output output/
 * @endcode
 *
 * @par Experiment sweep
 * @code{.sh}
 * ./embi_sim --experiment experiment.json
 * @endcode
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#pragma once

#include "core/Config.hpp"

#include <string>
#include <vector>

namespace embi {

/**
 * @struct CLIResult
 * @brief Parsed CLI output: either a single Config or an ExperimentConfig.
 */
struct CLIResult {
    Config         config;          ///< Single-run config (populated if !experiment_mode).
    ExperimentConfig exp_config;    ///< Sweep config (populated if experiment_mode).
    bool           experiment_mode{false}; ///< True if running a parameter sweep.
    bool           help_requested{false};  ///< True if --help was passed.
};

/**
 * @class CLI
 * @brief Command-line interface parser.
 *
 * @par Supported flags
 * | Flag                  | Default     | Description                           |
 * |-----------------------|-------------|---------------------------------------|
 * | --scheduler NAME      | embi        | Scheduler identifier                  |
 * | --workload NAME       | poisson     | Workload identifier                   |
 * | --profile NAME        | —           | Named workload profile (overrides above) |
 * | --ticks N             | 1000000     | Simulation ticks                      |
 * | --num-processes N     | 16          | Number of processes                   |
 * | --seed N              | 42          | PRNG seed                             |
 * | --arrival-rate F      | 0.5         | Mean arrival rate                     |
 * | --service-rate F      | 1.0         | Mean service rate                     |
 * | --alpha F             | 0.1         | EWMA alpha                            |
 * | --beta F              | 0.1         | EWMA beta                             |
 * | --M F                 | 10.0        | EMBI service-capacity bound           |
 * | --output DIR          | output      | Output directory                      |
 * | --log-freq N          | 1           | Log every N ticks                     |
 * | --binary-log          | false       | Use binary log format                 |
 * | --null-log            | false       | Use null logger (benchmarking)        |
 * | --trace FILE          | —           | Trace file for 'trace' workload       |
 * | --experiment FILE     | —           | JSON experiment config file           |
 * | --pareto-shape F      | 1.5         | Pareto shape (heavy-tail)             |
 * | --pareto-scale F      | 1.0         | Pareto scale (heavy-tail)             |
 * | --help / -h           | —           | Print help and exit                   |
 *
 * @complexity O(argc) for parse().
 */
class CLI {
public:
    CLI()                      = delete;
    CLI(const CLI&)            = delete;
    CLI& operator=(const CLI&) = delete;

    /**
     * @brief Parses argc/argv into a CLIResult.
     * @param argc  Argument count (from main).
     * @param argv  Argument vector (from main).
     * @return Populated CLIResult.
     * @throws std::invalid_argument on unknown flags or bad values.
     */
    [[nodiscard]] static CLIResult parse(int argc, char* argv[]);

    /**
     * @brief Prints full usage information to stdout.
     * @param prog  Program name (argv[0]).
     */
    static void printHelp(const std::string& prog);
};

} // namespace embi
