#include <gtest/gtest.h>
#include "core/OracleEvaluator.hpp"
#include "schedulers/FCFSScheduler.hpp"
#include "core/Config.hpp"
#include "core/Process.hpp"

namespace embi {

class OracleEMBITest : public ::testing::Test {
protected:
    Config config;
    GraphState state;
    std::unique_ptr<FCFSScheduler> scheduler;
    
    void SetUp() override {
        config.num_processes = 5;
        scheduler = std::make_unique<FCFSScheduler>(config);
    }
    
    std::vector<Process> make_processes(std::vector<int> queues) {
        std::vector<Process> p;
        for (std::size_t i = 0; i < queues.size(); ++i) {
            p.emplace_back(i, 0.5, 1.0, 0.1, 0.1);
            for (int j = 0; j < queues[i]; ++j) {
                p.back().arrival(0.0); // This increments queue_length automatically
            }
        }
        return p;
    }
};

TEST_F(OracleEMBITest, SingleRunnableJob) {
    auto procs = make_processes({1, 0});
    auto scores = OracleEvaluator::compute_oracle_embi(procs, state, *scheduler, config);
    EXPECT_EQ(scores[0], 0.0);
}

TEST_F(OracleEMBITest, TwoRunnableJobs_Independent) {
    auto procs = make_processes({1, 1});
    auto scores = OracleEvaluator::compute_oracle_embi(procs, state, *scheduler, config);
    // Since mu is identical and no dependencies, swapping order doesn't change total queue length integral.
    EXPECT_EQ(scores[0], 0.0);
    EXPECT_EQ(scores[1], 0.0);
}

TEST_F(OracleEMBITest, DAGWakeup_AvoidReleasingWork) {
    auto procs = make_processes({1, 0, 0, 1});
    ReconstructedGraphState r_state;
    // 0 -> 1 -> 2
    r_state.service_graph[0] = {1};
    r_state.service_graph[1] = {2};
    state.topology = &r_state;
    
    auto scores = OracleEvaluator::compute_oracle_embi(procs, state, *scheduler, config);
    
    // As manually derived, picking 3 first delays the arrival of 1 and 2, 
    // keeping the active queue length strictly smaller over time.
    // Thus forcing 3 yields lower total J than baseline (which picks 0 first).
    // EMBI(3) should be strictly positive.
    EXPECT_GT(scores[3], 0.0);
    // EMBI(0) should be 0 because it's the baseline choice.
    EXPECT_EQ(scores[0], 0.0);
}

TEST_F(OracleEMBITest, MultipleCounterfactuals) {
    auto procs = make_processes({1, 0, 1, 1});
    ReconstructedGraphState r_state;
    // 0 -> 1 (adds 1 job)
    r_state.service_graph[0] = {1};
    state.topology = &r_state;
    
    auto scores = OracleEvaluator::compute_oracle_embi(procs, state, *scheduler, config);
    
    // Both 2 and 3 have no dependents, meaning they don't release new work.
    // Picking them is better than picking 0.
    EXPECT_GT(scores[2], 0.0);
    EXPECT_GT(scores[3], 0.0);
    EXPECT_EQ(scores[2], scores[3]); // Symmetrical
}

} // namespace embi
