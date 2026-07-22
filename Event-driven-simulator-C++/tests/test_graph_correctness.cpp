/**
 * @file test_graph_correctness.cpp
 * @brief Unit tests for Graph Recovery and Katz propagation.
 *
 * @author  EMBI Simulator Project
 */

#include <gtest/gtest.h>
#include <algorithm>
#include "trace/TraceReconstructor.hpp"
#include "workloads/GraphWorkload.hpp"
#include "schedulers/estimators/KatzEMBIEstimator.hpp"

using namespace embi;

TEST(GraphCorrectness, DAGRecovery_Diamond) {
    auto trace = GraphWorkload::generate(GraphTopology::Diamond, 0, 100);
    
    TraceReconstructor reconstructor;
    auto reconstructed = reconstructor.reconstruct(trace);
    
    const auto& deps = reconstructed.service_graph;
    
    // A diamond graph: 0->1, 0->2, 1->3, 2->3
    // Meaning 3 depends on 1 and 2.
    ASSERT_TRUE(deps.count(1));
    ASSERT_TRUE(deps.count(2));
    
    EXPECT_NE(std::find(deps.at(1).begin(), deps.at(1).end(), 3), deps.at(1).end());
    EXPECT_NE(std::find(deps.at(2).begin(), deps.at(2).end(), 3), deps.at(2).end());
}

TEST(GraphCorrectness, KatzPropagation_Chain) {
    ReconstructedGraphState r_state;
    // 0 -> 1 -> 2
    r_state.service_graph[0] = {1};
    r_state.service_graph[1] = {2};

    GraphState state;
    state.topology = &r_state;
    
    std::vector<Process> procs;
    procs.emplace_back(0, 0.5, 1.0, 0.1, 0.1);
    procs.emplace_back(1, 0.5, 1.0, 0.1, 0.1);
    procs.emplace_back(2, 0.5, 1.0, 0.1, 0.1);
    
    procs[0].queue_length = 1;
    procs[1].queue_length = 1;
    procs[2].queue_length = 1;
    
    KatzEMBIEstimator estimator(0.5, 10, 1e-4);
    auto c = estimator.estimate(procs, state);
    
    // C_2 = 1.0
    // C_1 = 0.5 * 1.0 + 1 = 1.5
    // C_0 = 0.5 * 1.5 + 1 = 1.75
    EXPECT_NEAR(c[2], 1.0, 1e-9);
    EXPECT_NEAR(c[1], 1.5, 1e-9);
    EXPECT_NEAR(c[0], 1.75, 1e-9);
}

TEST(GraphCorrectness, KatzAnalytical_A_B_C) {
    ReconstructedGraphState r_state;
    r_state.service_graph[0] = {1};
    r_state.service_graph[1] = {2};

    GraphState state;
    state.topology = &r_state;
    
    std::vector<Process> procs;
    procs.emplace_back(0, 0.5, 1.0, 0.1, 0.1);
    procs.emplace_back(1, 0.5, 1.0, 0.1, 0.1);
    procs.emplace_back(2, 0.5, 1.0, 0.1, 0.1);
    
    // b = [3, 2, 1]
    procs[0].queue_length = 3;
    procs[1].queue_length = 2;
    procs[2].queue_length = 1;
    
    KatzEMBIEstimator estimator(0.5, 50, 1e-9);
    auto c = estimator.estimate(procs, state);
    
    // c_2 = 1.0
    // c_1 = 2.0 + 0.5 * 1.0 = 2.5
    // c_0 = 3.0 + 0.5 * 2.5 = 4.25
    EXPECT_NEAR(c[2], 1.0, 1e-9);
    EXPECT_NEAR(c[1], 2.5, 1e-9);
    EXPECT_NEAR(c[0], 4.25, 1e-9);
}

TEST(GraphCorrectness, KatzConvergence_FailsWhenAlphaTooLarge) {
    ReconstructedGraphState r_state;
    // Cycle: 0 -> 1 -> 0
    r_state.service_graph[0] = {1};
    r_state.service_graph[1] = {0};

    GraphState state;
    state.topology = &r_state;
    
    std::vector<Process> procs;
    procs.emplace_back(0, 0.5, 1.0, 0.1, 0.1);
    procs.emplace_back(1, 0.5, 1.0, 0.1, 0.1);
    procs[0].queue_length = 1;
    procs[1].queue_length = 1;
    
    // alpha = 1.5 > 1, so with a cycle this should diverge
    KatzEMBIEstimator estimator(1.5, 100, 1e-4);
    auto c = estimator.estimate(procs, state);
    
    // Values should be very large or Inf
    EXPECT_GT(c[0], 1000.0);
    EXPECT_GT(c[1], 1000.0);
}

TEST(GraphCorrectness, GraphRobustness_Empty) {
    GraphState state;
    state.topology = nullptr; // Empty state
    std::vector<Process> procs;
    
    KatzEMBIEstimator estimator(0.5, 10, 1e-4);
    auto c = estimator.estimate(procs, state);
    EXPECT_TRUE(c.empty());
}

TEST(GraphCorrectness, GraphRobustness_SingleNode) {
    ReconstructedGraphState r_state;
    GraphState state;
    state.topology = &r_state;
    
    std::vector<Process> procs;
    procs.emplace_back(0, 0.5, 1.0, 0.1, 0.1);
    procs[0].queue_length = 5;
    
    KatzEMBIEstimator estimator(0.5, 10, 1e-4);
    auto c = estimator.estimate(procs, state);
    
    ASSERT_EQ(c.size(), 1);
    EXPECT_NEAR(c[0], 5.0, 1e-9);
}

TEST(GraphCorrectness, GraphRobustness_Disconnected) {
    ReconstructedGraphState r_state;
    r_state.service_graph[0] = {1}; // 0->1
    // 2 is disconnected
    GraphState state;
    state.topology = &r_state;
    
    std::vector<Process> procs;
    procs.emplace_back(0, 0.5, 1.0, 0.1, 0.1);
    procs.emplace_back(1, 0.5, 1.0, 0.1, 0.1);
    procs.emplace_back(2, 0.5, 1.0, 0.1, 0.1);
    procs[0].queue_length = 1;
    procs[1].queue_length = 1;
    procs[2].queue_length = 5;
    
    KatzEMBIEstimator estimator(0.5, 10, 1e-9);
    auto c = estimator.estimate(procs, state);
    
    EXPECT_NEAR(c[2], 5.0, 1e-9);
    EXPECT_NEAR(c[1], 1.0, 1e-9);
    EXPECT_NEAR(c[0], 1.5, 1e-9);
}

TEST(GraphCorrectness, GraphRobustness_FanOut) {
    ReconstructedGraphState r_state;
    r_state.service_graph[0] = {1, 2}; // 0 -> 1 and 0 -> 2
    GraphState state;
    state.topology = &r_state;
    
    std::vector<Process> procs;
    procs.emplace_back(0, 0.5, 1.0, 0.1, 0.1);
    procs.emplace_back(1, 0.5, 1.0, 0.1, 0.1);
    procs.emplace_back(2, 0.5, 1.0, 0.1, 0.1);
    procs[0].queue_length = 1;
    procs[1].queue_length = 2;
    procs[2].queue_length = 3;
    
    KatzEMBIEstimator estimator(0.5, 10, 1e-9);
    auto c = estimator.estimate(procs, state);
    
    EXPECT_NEAR(c[1], 2.0, 1e-9);
    EXPECT_NEAR(c[2], 3.0, 1e-9);
    EXPECT_NEAR(c[0], 1.0 + 0.5*(2.0 + 3.0), 1e-9); // 1 + 2.5 = 3.5
}

TEST(GraphCorrectness, GraphRobustness_FanIn) {
    ReconstructedGraphState r_state;
    r_state.service_graph[0] = {2}; 
    r_state.service_graph[1] = {2}; 
    GraphState state;
    state.topology = &r_state;
    
    std::vector<Process> procs;
    procs.emplace_back(0, 0.5, 1.0, 0.1, 0.1);
    procs.emplace_back(1, 0.5, 1.0, 0.1, 0.1);
    procs.emplace_back(2, 0.5, 1.0, 0.1, 0.1);
    procs[0].queue_length = 1;
    procs[1].queue_length = 2;
    procs[2].queue_length = 3;
    
    KatzEMBIEstimator estimator(0.5, 10, 1e-9);
    auto c = estimator.estimate(procs, state);
    
    EXPECT_NEAR(c[2], 3.0, 1e-9);
    EXPECT_NEAR(c[0], 1.0 + 0.5*(3.0), 1e-9); // 2.5
    EXPECT_NEAR(c[1], 2.0 + 0.5*(3.0), 1e-9); // 3.5
}
