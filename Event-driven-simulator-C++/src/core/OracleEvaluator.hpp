#pragma once
#include "core/Process.hpp"
#include "core/GraphState.hpp"
#include "schedulers/BaseScheduler.hpp"
#include "core/Config.hpp"
#include <vector>
#include <optional>

namespace embi {

class OracleEvaluator {
public:
    /**
     * @brief Computes total residual waiting time J if we run the system to completion 
     *        under the given baseline scheduler.
     * @param procs The current snapshot of processes.
     * @param state The current graph/lock state.
     * @param scheduler The baseline scheduler policy.
     * @param config The simulation config.
     * @param forced_first_pid If set, forces this PID to execute the FIRST quantum.
     * @return Total expected waiting time J over all remaining jobs.
     */
    static double evaluate_J(const std::vector<Process>& procs,
                             const GraphState& state,
                             BaseScheduler& scheduler,
                             const Config& config,
                             std::optional<std::size_t> forced_first_pid = std::nullopt);

    /**
     * @brief Computes Oracle EMBI for all processes.
     * EMBI(i) = J(baseline) - J(baseline_forced_i)
     */
    static std::vector<double> compute_oracle_embi(const std::vector<Process>& procs,
                                                   const GraphState& state,
                                                   BaseScheduler& scheduler,
                                                   const Config& config);
};

} // namespace embi
