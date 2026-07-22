#include <gtest/gtest.h>
#include "schedulers/estimators/KatzEMBIEstimator.hpp"
#include "schedulers/EMBIScheduler.hpp"
#include "schedulers/FCFSScheduler.hpp"
#include "core/Process.hpp"
#include "core/Simulator.hpp"
#include "core/OracleEvaluator.hpp"
#include <vector>

namespace embi {

class RobustnessTest : public ::testing::Test {
protected:
    Config cfg;
    std::vector<Process> procs;
    ReconstructedGraphState r_state;
    GraphState state;
    
    void SetUp() override {
        cfg.workload_name = "synthetic";
        cfg.alpha = 0.5;
        state.topology = &r_state;
    }
    
    std::vector<Process> make_procs(int n) {
        std::vector<Process> p;
        for (int i=0; i<n; ++i) {
            Process proc(i, 0.5, 1.0, 0.1, 0.1, 10.0, 0);
            proc.arrival(0.0);
            p.push_back(proc);
        }
        return p;
    }
};

// 1. Cyclic Graph Robustness
TEST_F(RobustnessTest, KatzHandlesCycles) {
    procs = make_procs(3);
    // 0 -> 1 -> 2 -> 0
    r_state.service_graph[0] = {1};
    r_state.service_graph[1] = {2};
    r_state.service_graph[2] = {0};
    
    KatzEMBIEstimator estimator(0.5, 50, 1e-4);
    auto scores = estimator.estimate(procs, state);
    
    // Cycles should safely converge if alpha * spectral_radius < 1.
    // For a 3-cycle, spectral radius is 1. Alpha is 0.5.
    // Scores should be well-defined and positive.
    EXPECT_EQ(scores.size(), 3);
    EXPECT_GT(scores[0], 0.0);
    EXPECT_GT(scores[1], 0.0);
    EXPECT_GT(scores[2], 0.0);
    
    // By symmetry, all scores should be equal
    EXPECT_NEAR(scores[0], scores[1], 1e-4);
    EXPECT_NEAR(scores[1], scores[2], 1e-4);
}

// 2. Disconnected Components
TEST_F(RobustnessTest, DisconnectedComponents) {
    procs = make_procs(4);
    // 0 -> 1, 2 and 3 are isolated
    r_state.service_graph[0] = {1};
    
    KatzEMBIEstimator estimator(0.5, 10, 1e-4);
    auto scores = estimator.estimate(procs, state);
    
    // 0 should have higher score than 1 (which gets baseline)
    EXPECT_GT(scores[0], scores[1]);
    
    // 2 and 3 should just have baseline score (1.0)
    EXPECT_NEAR(scores[2], 1.0, 1e-6);
    EXPECT_NEAR(scores[3], 1.0, 1e-6);
}

// 3. Extreme Alpha Weights
TEST_F(RobustnessTest, ExtremeAlphaWeights) {
    procs = make_procs(2);
    r_state.service_graph[0] = {1};
    
    // Very small alpha -> score dominated by baseline (1.0)
    KatzEMBIEstimator small_alpha(1e-6, 10, 1e-4);
    auto scores_small = small_alpha.estimate(procs, state);
    EXPECT_NEAR(scores_small[0], 1.0, 1e-4);
    
    // Large alpha -> score dominated by graph structure
    // (Ensure it doesn't crash or go infinite for a DAG)
    KatzEMBIEstimator large_alpha(100.0, 10, 1e-4);
    auto scores_large = large_alpha.estimate(procs, state);
    EXPECT_GT(scores_large[0], 100.0); // 1.0 + 100 * 1.0 = 101.0
}

// 4. Simulation Invariant (Conservation Law of total work)
TEST_F(RobustnessTest, SimulationInvariantConservationLaw) {
    // 5 independent jobs, all 1-tick.
    procs = make_procs(5);
    // Arbitrary arrive times so FCFS vs EMBI differs in sequence
    procs[0].arrival(1.0);
    procs[1].arrival(0.5);
    procs[2].arrival(2.0);
    procs[3].arrival(0.0);
    procs[4].arrival(1.5);
    
    // FCFS Evaluator
    FCFSScheduler fcfs(cfg);
    double J_fcfs = OracleEvaluator::evaluate_J(procs, state, fcfs, cfg, std::nullopt);
    
    // EMBI Evaluator (using Katz)
    auto estimator = std::make_unique<KatzEMBIEstimator>(0.5, 10, 1e-4);
    EMBIScheduler embi(std::move(estimator));
    double J_embi = OracleEvaluator::evaluate_J(procs, state, embi, cfg, std::nullopt);
    
    // Since this is a single-server unit-capacity closed system, J is strictly invariant 
    // across ANY work-conserving policy! 
    EXPECT_NEAR(J_fcfs, J_embi, 1e-6);
}

} // namespace embi
