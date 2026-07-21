#include "core/Simulator.hpp"
#include "core/Config.hpp"
#include "schedulers/HybridEMBIScheduler.hpp"

#include <iostream>
#include <string>
#include <vector>

using namespace embi;

void assertSymmetry() {
    Config cfg;
    cfg.scheduler_name = "hybrid_embi";
    cfg.num_processes = 3;
    cfg.ticks = 10;
    cfg.M = 10.0;
    cfg.epsilon_total = 0.05;
    cfg.tau_constant_bound = 0.5;

    // Simulate symmetry: all lambda and mu identical
    std::vector<Process> processes;
    processes.reserve(3);
    for (int i=0; i<3; ++i) processes.push_back(Process(i, 0.5, 0.5, 0.1, 0.1));
    for (int i=0; i<3; ++i) {
        processes[i].id = i;
        processes[i].lambda_hat = 0.5;
        processes[i].mu_hat = 0.5;
        processes[i].queue_length = 5;
    }
    OnlineSnapshot stats;
    SchedulerContext ctx{processes, 10.0, std::nullopt, stats, cfg};

    HybridEMBIScheduler hybrid(cfg);
    Decision d = hybrid.choose(ctx);

    std::cout << "Scenario: Symmetry\n";
    std::cout << "Expected: Fallback\n";
    std::cout << "Observed: " << (d.mode_flag == 1 ? "Fallback" : "EMBI") << "\n";
    if (d.mode_flag == 1) {
        std::cout << "PASS\n\n";
    } else {
        std::cout << "FAIL\n\n";
        exit(1);
    }
}

void assertStrongSignal() {
    Config cfg;
    cfg.scheduler_name = "hybrid_embi";
    cfg.num_processes = 3;
    cfg.ticks = 10;
    cfg.M = 10.0;
    cfg.epsilon_total = 0.05;
    cfg.tau_constant_bound = 0.0; // keep tau small for the test

    std::vector<Process> processes;
    processes.reserve(3);
    for (int i=0; i<3; ++i) processes.push_back(Process(i, 0.5, 0.5, 0.1, 0.1));
    // P0 strong signal
    processes[0].id = 0;
    processes[0].lambda_hat = 0.9;
    processes[0].mu_hat = 1.0;
    processes[0].queue_length = 20;

    // P1 weak
    processes[1].id = 1;
    processes[1].lambda_hat = 0.5;
    processes[1].mu_hat = 0.5;
    processes[1].queue_length = 5;

    // P2 weak
    processes[2].id = 2;
    processes[2].lambda_hat = 0.1;
    processes[2].mu_hat = 0.1;
    processes[2].queue_length = 1;

    OnlineSnapshot stats;
    SchedulerContext ctx{processes, 10.0, std::nullopt, stats, cfg};

    HybridEMBIScheduler hybrid(cfg);
    Decision d = hybrid.choose(ctx);

    std::cout << "Scenario: Strong Signal\n";
    std::cout << "Expected: EMBI\n";
    std::cout << "Observed: " << (d.mode_flag == 2 ? "EMBI" : "Fallback") << "\n";
    if (d.mode_flag == 2) {
        std::cout << "PASS\n\n";
    } else {
        std::cout << "FAIL\n\n";
        exit(1);
    }
}

void assertEmptyQueues() {
    Config cfg;
    cfg.scheduler_name = "hybrid_embi";
    cfg.num_processes = 3;
    
    std::vector<Process> processes;
    processes.reserve(3);
    for (int i=0; i<3; ++i) processes.push_back(Process(i, 0.5, 0.5, 0.1, 0.1));
    for (int i=0; i<3; ++i) {
        processes[i].id = i;
        processes[i].queue_length = 0; // empty
    }
    
    OnlineSnapshot stats;
    SchedulerContext ctx{processes, 10.0, std::nullopt, stats, cfg};

    HybridEMBIScheduler hybrid(cfg);
    Decision d = hybrid.choose(ctx);

    std::cout << "Scenario: Empty Queues\n";
    std::cout << "Expected: valid=false (idle)\n";
    std::cout << "Observed: " << (!d.valid ? "idle" : "scheduled") << "\n";
    if (!d.valid) {
        std::cout << "PASS\n\n";
    } else {
        std::cout << "FAIL\n\n";
        exit(1);
    }
}

int main(int argc, char** argv) {
    std::cout << "====================================\n";
    std::cout << "       EMBI THEOREM VERIFIER        \n";
    std::cout << "====================================\n\n";
    
    if (argc > 1) {
        std::string arg = argv[1];
        if (arg == "--scenario") {
            std::string scenario = argv[2];
            if (scenario == "symmetry") assertSymmetry();
            else if (scenario == "strong_signal") assertStrongSignal();
            else if (scenario == "empty_queues") assertEmptyQueues();
            else std::cout << "Unknown scenario\n";
            return 0;
        }
    }

    assertSymmetry();
    assertStrongSignal();
    assertEmptyQueues();

    return 0;
}
