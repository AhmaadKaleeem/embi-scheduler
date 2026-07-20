/**
 * @file main.cpp
 * @brief EMBI CPU Scheduling Simulator entry point.
 *
 * Parses CLI arguments, dispatches to either single-run (Simulator) or
 * multi-run (Experiment) mode, and exits with a meaningful code.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#include "CLI.hpp"
#include "core/Experiment.hpp"
#include "core/Simulator.hpp"
#include "utils/Timer.hpp"

#include <cstdlib>
#include <exception>
#include <iostream>
#include <iomanip>

int main(int argc, char* argv[]) {
    try {
        // ── Parse CLI ────────────────────────────────────────────────────────
        auto result = embi::CLI::parse(argc, argv);

        if (result.help_requested) {
            embi::CLI::printHelp(argv[0]);
            return EXIT_SUCCESS;
        }

        embi::Timer total_timer;

        if (result.experiment_mode) {
            // ── Experiment sweep ─────────────────────────────────────────────
            std::cout << "== EMBI Scheduling Simulator - Experiment Mode ==\n"
                      << "Runs: " << result.exp_config.totalRuns() << "\n\n";

            embi::Experiment experiment(result.exp_config);
            experiment.run([](std::size_t completed, std::size_t total,
                               const embi::Results& /*r*/) {
                std::cout << "  Progress: " << completed << "/" << total << "\n";
            });

            std::cout << "\n== Experiment Complete in "
                      << std::fixed << std::setprecision(2)
                      << total_timer.elapsed_ms() / 1000.0 << " s ==\n";

        } else {
            // ── Single run ───────────────────────────────────────────────────
            const embi::Config& cfg = result.config;

            std::cout << "== EMBI Scheduling Simulator - Single Run ==\n"
                      << "  Scheduler  : " << cfg.scheduler_name   << "\n"
                      << "  Workload   : " << cfg.workload_name    << "\n"
                      << "  Ticks      : " << cfg.ticks            << "\n"
                      << "  Processes  : " << cfg.num_processes     << "\n"
                      << "  Seed       : " << cfg.seed             << "\n"
                      << "  Arrival λ  : " << cfg.arrival_rate     << "\n"
                      << "  M          : " << cfg.M                << "\n\n";

            embi::Simulator sim(cfg);
            auto results = sim.run();

            double elapsed_ms = total_timer.elapsed_ms();

            std::cout << std::fixed << std::setprecision(4)
                      << "\n== Results ==\n"
                      << "  Avg Waiting Time  : " << results.offline.avg_waiting_time   << " ticks\n"
                      << "  P99 Waiting Time  : " << results.offline.p99_waiting_time   << " ticks\n"
                      << "  Throughput        : " << results.online.throughput           << " jobs/tick\n"
                      << "  CPU Utilization   : " << results.online.utilization * 100.0  << " %\n"
                      << "  Jain Fairness     : " << results.offline.jain_fairness_index << "\n"
                      << "  Lyapunov V(final) : " << results.online.lyapunov_v          << "\n\n"
                      << "== Completed in " << elapsed_ms / 1000.0 << " s ==\n"
                      << "== Output in    " << cfg.output_dir << " ==\n";
        }

        return EXIT_SUCCESS;

    } catch (const std::invalid_argument& e) {
        std::cerr << "[ERROR] Invalid argument: " << e.what() << "\n"
                  << "Run with --help for usage.\n";
        return EXIT_FAILURE;
    } catch (const std::runtime_error& e) {
        std::cerr << "[ERROR] Runtime error: " << e.what() << "\n";
        return EXIT_FAILURE;
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] " << e.what() << "\n";
        return EXIT_FAILURE;
    }
}
