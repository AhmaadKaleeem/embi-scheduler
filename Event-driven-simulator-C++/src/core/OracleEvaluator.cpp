#include "core/OracleEvaluator.hpp"
#include <unordered_set>
#include <stdexcept>

namespace embi {

double OracleEvaluator::evaluate_J(const std::vector<Process>& procs,
                                   const GraphState& state,
                                   BaseScheduler& scheduler,
                                   const Config& config,
                                   std::optional<std::size_t> forced_first_pid) {
    std::vector<Process> p_clone = procs;
    GraphState s_clone = state;
    OnlineSnapshot snap{};
    
    double total_J = 0.0;
    bool is_first_tick = true;
    double current_tick = 0.0;

    // Simulation loop to drain queues
    int safety = 0;
    while (safety++ < 100000) {
        SchedulerContext ctx{p_clone, current_tick, std::nullopt, snap, config};
        ctx.graph_state = &s_clone;
        
        Decision d;
        if (is_first_tick && forced_first_pid.has_value()) {
            d.valid = true;
            d.chosen_pid = forced_first_pid.value();
            d.chosen_score = 0.0;
            // Ensure forced PID is actually runnable
            if (p_clone[d.chosen_pid].queue_length == 0 ||
                s_clone.blocked_jobs.count(d.chosen_pid) > 0) {
                return 1e9; // Invalid forced move
            }
        } else {
            d = scheduler.choose(ctx);
        }
        
        is_first_tick = false;

        if (!d.valid) {
            break;
        }

        std::size_t chosen = d.chosen_pid;
        
        if (p_clone[chosen].queue_length > 0) {
            p_clone[chosen].service(current_tick);
            
            // Wake up dependents if process finishes all current jobs
            if (p_clone[chosen].queue_length == 0 && s_clone.topology != nullptr) {
                auto it = s_clone.topology->service_graph.find(chosen);
                if (it != s_clone.topology->service_graph.end()) {
                    for (auto dep : it->second) {
                        if (dep < p_clone.size()) {
                            // If dep is already in the system and blocked, unblock it.
                            if (s_clone.blocked_jobs.erase(dep) == 0) {
                                // If it wasn't blocked, it's a DAG arrival.
                                p_clone[dep].arrival(current_tick); 
                            }
                        }
                    }
                }
            }
        }

        // Accumulate waiting time
        for (const auto& p : p_clone) {
            total_J += p.queue_length;
        }
        
        current_tick += 1.0;
    }
    
    // printf("evaluate_J(force=%d) = %f\n", forced_first_pid.value_or(-1), total_J);
    return total_J;
}

std::vector<double> OracleEvaluator::compute_oracle_embi(const std::vector<Process>& procs,
                                                         const GraphState& state,
                                                         BaseScheduler& scheduler,
                                                         const Config& config) {
    std::vector<double> embi_scores(procs.size(), 0.0);
    double baseline_J = evaluate_J(procs, state, scheduler, config);

    for (std::size_t i = 0; i < procs.size(); ++i) {
        if (procs[i].queue_length > 0 && state.blocked_jobs.count(i) == 0) {
            double forced_J = evaluate_J(procs, state, scheduler, config, i);
            embi_scores[i] = baseline_J - forced_J;
            printf("i=%zu baseline_J=%f forced_J=%f EMBI=%f\n", i, baseline_J, forced_J, embi_scores[i]);
        } else {
            embi_scores[i] = 0.0;
        }
    }
    return embi_scores;
}

} // namespace embi
