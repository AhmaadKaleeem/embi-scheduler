/**
 * @file Results.hpp
 * @brief Results container from a single simulation run.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#pragma once

#include "core/OfflineMetrics.hpp"
#include "schedulers/SchedulerContext.hpp"  // for OnlineSnapshot

#include <cstdint>
#include <string>
#include <vector>

namespace embi {

/**
 * @struct Results
 * @brief Complete output of one Simulator::run() call.
 *
 * Passed from Simulator to Experiment for aggregation, and from Experiment
 * to SummaryWriter for comparative reporting.
 */
struct Results {
    std::string    scheduler_name;  ///< Name of the scheduler used.
    std::string    workload_name;   ///< Name of the workload used.
    uint64_t       seed{0};         ///< PRNG seed used.
    double         arrival_rate{0.0}; ///< Configured arrival rate.
    std::vector<double> arrival_rate_asymmetric; ///< Optional per-process rates.
    Config         config;          ///< Full configuration used for the run.

    OnlineSnapshot online;          ///< Final online metrics snapshot.
    OfflineReport  offline;         ///< Post-simulation offline report.

    /// Returns true if the run completed without error.
    [[nodiscard]] bool valid() const noexcept {
        return !scheduler_name.empty();
    }
};

} // namespace embi
