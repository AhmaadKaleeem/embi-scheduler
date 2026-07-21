/**
 * @file CLI.cpp
 * @brief Command-line argument parser implementation.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#include "CLI.hpp"

#include "utils/ConfigLoader.hpp"

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace embi {

namespace {

void expect_arg(const char* flag, int i, int argc) {
    if (i >= argc) {
        throw std::invalid_argument(
            std::string("CLI: flag '") + flag + "' requires a value");
    }
}

double toDouble(const char* flag, const char* val) {
    try {
        std::size_t pos;
        double result = std::stod(std::string(val), &pos);
        if (pos != std::string(val).size()) throw std::exception{};
        return result;
    } catch (...) {
        throw std::invalid_argument(
            std::string("CLI: '") + flag + "' has invalid numeric value '" + val + "'");
    }
}

uint64_t toUInt64(const char* flag, const char* val) {
    try {
        std::size_t pos;
        uint64_t result = static_cast<uint64_t>(std::stoull(std::string(val), &pos));
        if (pos != std::string(val).size()) throw std::exception{};
        return result;
    } catch (...) {
        throw std::invalid_argument(
            std::string("CLI: '") + flag + "' has invalid integer value '" + val + "'");
    }
}

} // anonymous namespace

CLIResult CLI::parse(int argc, char* argv[]) {
    CLIResult result;
    Config&   cfg = result.config;

    // If a config file is specified, load it first (then override with CLI flags)
    bool config_file_loaded = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            result.help_requested = true;
            return result;
        }

        // ── Config file ───────────────────────────────────────────────────────
        if (arg == "--config") {
            expect_arg(arg.c_str(), ++i, argc);
            cfg = ConfigLoader::load(argv[i]);
            config_file_loaded = true;
            continue;
        }

        // ── Experiment sweep ──────────────────────────────────────────────────
        if (arg == "--experiment") {
            expect_arg(arg.c_str(), ++i, argc);
            result.exp_config      = ConfigLoader::loadExperiment(argv[i]);
            result.experiment_mode = true;
            continue;
        }

        // ── Scheduler ─────────────────────────────────────────────────────────
        if (arg == "--scheduler") {
            expect_arg(arg.c_str(), ++i, argc);
            cfg.scheduler_name = argv[i];
            continue;
        }
        if (arg == "--no-clip") {
            cfg.embi_clipped = false;
            cfg.scheduler_name = "embi_unclipped";
            continue;
        }

        // ── Workload ──────────────────────────────────────────────────────────
        if (arg == "--workload") {
            expect_arg(arg.c_str(), ++i, argc);
            cfg.workload_name = argv[i];
            continue;
        }
        if (arg == "--profile") {
            expect_arg(arg.c_str(), ++i, argc);
            cfg.workload_profile = std::string(argv[i]);
            continue;
        }
        if (arg == "--trace") {
            expect_arg(arg.c_str(), ++i, argc);
            cfg.trace_file = std::string(argv[i]);
            continue;
        }

        // ── Simulation ────────────────────────────────────────────────────────
        if (arg == "--ticks") {
            expect_arg(arg.c_str(), ++i, argc);
            cfg.ticks = toUInt64(arg.c_str(), argv[i]);
            continue;
        }
        if (arg == "--num-processes") {
            expect_arg(arg.c_str(), ++i, argc);
            cfg.num_processes = static_cast<std::size_t>(toUInt64(arg.c_str(), argv[i]));
            continue;
        }
        if (arg == "--seed") {
            expect_arg(arg.c_str(), ++i, argc);
            cfg.seed = toUInt64(arg.c_str(), argv[i]);
            continue;
        }
        if (arg == "--warmup-ticks") {
            expect_arg(arg.c_str(), ++i, argc);
            cfg.warmup_ticks = toUInt64(arg.c_str(), argv[i]);
            continue;
        }
        if (arg == "--context-switch-cost") {
            expect_arg(arg.c_str(), ++i, argc);
            cfg.context_switch_cost = toUInt64(arg.c_str(), argv[i]);
            continue;
        }
        if (arg == "--git-commit") {
            expect_arg(arg.c_str(), ++i, argc);
            cfg.git_commit_hash = argv[i];
            continue;
        }
        if (arg == "--binary-hash") {
            expect_arg(arg.c_str(), ++i, argc);
            cfg.binary_sha256 = argv[i];
            continue;
        }
        if (arg == "--config-hash") {
            expect_arg(arg.c_str(), ++i, argc);
            cfg.config_hash = argv[i];
            continue;
        }

        // ── Rate parameters ───────────────────────────────────────────────────
        if (arg == "--arrival-rate-asymmetric") {
            expect_arg(arg.c_str(), ++i, argc);
            std::string val = argv[i];
            std::stringstream ss(val);
            std::string token;
            cfg.arrival_rate_asymmetric.clear();
            while (std::getline(ss, token, ',')) {
                cfg.arrival_rate_asymmetric.push_back(toDouble(arg.c_str(), token.c_str()));
            }
            continue;
        }
        if (arg == "--alpha-sweep") {
            expect_arg(arg.c_str(), ++i, argc);
            std::string val = argv[i];
            std::stringstream ss(val);
            std::string token;
            result.exp_config.alphas.clear();
            while (std::getline(ss, token, ',')) {
                result.exp_config.alphas.push_back(toDouble(arg.c_str(), token.c_str()));
            }
            result.experiment_mode = true;
            continue;
        }
        if (arg == "--arrival-rate") {
            expect_arg(arg.c_str(), ++i, argc);
            cfg.arrival_rate = toDouble(arg.c_str(), argv[i]);
            continue;
        }
        if (arg == "--service-rate") {
            expect_arg(arg.c_str(), ++i, argc);
            cfg.service_rate = toDouble(arg.c_str(), argv[i]);
            continue;
        }
        if (arg == "--alpha") {
            expect_arg(arg.c_str(), ++i, argc);
            cfg.alpha = toDouble(arg.c_str(), argv[i]);
            continue;
        }
        if (arg == "--beta") {
            expect_arg(arg.c_str(), ++i, argc);
            cfg.beta = toDouble(arg.c_str(), argv[i]);
            continue;
        }
        if (arg == "--lambda-noise") {
            expect_arg(arg.c_str(), ++i, argc);
            cfg.lambda_noise_stddev = toDouble(arg.c_str(), argv[i]);
            continue;
        }
        if (arg == "--M") {
            expect_arg(arg.c_str(), ++i, argc);
            cfg.M = toDouble(arg.c_str(), argv[i]);
            continue;
        }
        if (arg == "--epsilon-total") {
            expect_arg(arg.c_str(), ++i, argc);
            cfg.epsilon_total = toDouble(arg.c_str(), argv[i]);
            continue;
        }
        if (arg == "--tau-constant") {
            expect_arg(arg.c_str(), ++i, argc);
            cfg.tau_constant_bound = toDouble(arg.c_str(), argv[i]);
            continue;
        }
        // ── Workload-specific ─────────────────────────────────────────────────
        if (arg == "--pareto-shape") {
            expect_arg(arg.c_str(), ++i, argc);
            cfg.pareto_shape = toDouble(arg.c_str(), argv[i]);
            continue;
        }
        if (arg == "--pareto-scale") {
            expect_arg(arg.c_str(), ++i, argc);
            cfg.pareto_scale = toDouble(arg.c_str(), argv[i]);
            continue;
        }
        if (arg == "--uniform-lo") {
            expect_arg(arg.c_str(), ++i, argc);
            cfg.uniform_lo = toDouble(arg.c_str(), argv[i]);
            continue;
        }
        if (arg == "--uniform-hi") {
            expect_arg(arg.c_str(), ++i, argc);
            cfg.uniform_hi = toDouble(arg.c_str(), argv[i]);
            continue;
        }
        if (arg == "--burst-on-rate") {
            expect_arg(arg.c_str(), ++i, argc);
            cfg.burst_on_rate = toDouble(arg.c_str(), argv[i]);
            continue;
        }
        if (arg == "--burst-off-rate") {
            expect_arg(arg.c_str(), ++i, argc);
            cfg.burst_off_rate = toDouble(arg.c_str(), argv[i]);
            continue;
        }

        // ── Logging ───────────────────────────────────────────────────────────
        if (arg == "--output") {
            expect_arg(arg.c_str(), ++i, argc);
            cfg.output_dir = argv[i];
            continue;
        }
        if (arg == "--log-freq") {
            expect_arg(arg.c_str(), ++i, argc);
            cfg.log_freq = toUInt64(arg.c_str(), argv[i]);
            continue;
        }
        if (arg == "--binary-log") {
            cfg.binary_log = true;
            continue;
        }
        if (arg == "--null-log") {
            cfg.null_log = true;
            continue;
        }
        if (arg == "--human-trace") {
            cfg.human_trace = true;
            continue;
        }

        // ── Lock-contention workload ───────────────────────────────────────────
        if (arg == "--num-locks") {
            expect_arg(arg.c_str(), ++i, argc);
            cfg.num_locks = static_cast<std::size_t>(toUInt64(arg.c_str(), argv[i]));
            continue;
        }
        if (arg == "--lock-rate") {
            expect_arg(arg.c_str(), ++i, argc);
            cfg.lock_request_rate = toDouble(arg.c_str(), argv[i]);
            continue;
        }
        if (arg == "--lock-hold") {
            expect_arg(arg.c_str(), ++i, argc);
            cfg.lock_hold_mean = toDouble(arg.c_str(), argv[i]);
            continue;
        }

        throw std::invalid_argument("CLI: unknown flag '" + arg + "'");
    }

    // Validate if not in experiment mode
    if (!result.experiment_mode && !result.help_requested) {
        if (cfg.warmup_ticks == 0) {
            uint64_t ten_percent = cfg.ticks / 10;
            cfg.warmup_ticks = std::max<uint64_t>(100000, ten_percent);
        }
        cfg.validate();
    }

    (void)config_file_loaded;
    return result;
}

void CLI::printHelp(const std::string& prog) {
    std::cout <<
        "EMBI CPU Scheduling Simulator\n"
        "Usage: " << prog << " [OPTIONS]\n\n"
        "Simulation options:\n"
        "  --scheduler NAME       Scheduler: embi, embi_unclipped, maxweight, cmu, rr, fcfs [default: embi]\n"
        "  --workload  NAME       Workload:  uniform, poisson, bursty, heavy_tail, trace [default: poisson]\n"
        "  --profile   NAME       Named profile: web_server, database, cloud_vm, embedded, real_time, interactive_desktop\n"
        "  --ticks     N          Simulation ticks [default: 1000000]\n"
        "  --num-processes N      Number of processes [default: 16]\n"
        "  --seed      N          PRNG seed [default: 42]\n"
        "  --arrival-rate F       Mean arrivals/tick [default: 0.5]\n"
        "  --service-rate F       Mean service rate [default: 1.0]\n"
        "  --alpha     F          EWMA arrival smoothing (0,1) [default: 0.1]\n"
        "  --beta      F          EWMA service smoothing (0,1) [default: 0.1]\n"
        "  --M         F          EMBI capacity bound [default: 10.0]\n"
        "  --no-clip              Use unclipped EMBI variant\n\n"
        "Workload-specific:\n"
        "  --trace FILE           Trace CSV file for 'trace' workload\n"
        "  --pareto-shape F       Pareto shape alpha [default: 1.5]\n"
        "  --pareto-scale F       Pareto minimum x_m [default: 1.0]\n"
        "  --uniform-lo F         U(lo, hi) lower bound [default: 0.5]\n"
        "  --uniform-hi F         U(lo, hi) upper bound [default: 2.0]\n\n"
        "Output:\n"
        "  --output    DIR        Output directory [default: output]\n"
        "  --log-freq  N          Log every N ticks [default: 1]\n"
        "  --binary-log           Use packed binary log format\n"
        "  --null-log             Discard all log output (benchmarking)\n\n"
        "Config / Experiment:\n"
        "  --config    FILE       Load configuration from YAML or JSON file\n"
        "  --experiment FILE      Run a parameter sweep from JSON config\n\n"
        "  --help / -h            Show this message\n";
}

} // namespace embi
