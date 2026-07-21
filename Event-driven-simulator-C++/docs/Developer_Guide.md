# Developer Guide

This document defines the architecture, execution pipeline, and extension interfaces for the EMBI Scheduling Simulator.

## Architecture and Execution Pipeline

The simulator implements a Discrete-Event Simulation architecture.

1.  Configuration and Interface (CLI.cpp, ConfigLoader.cpp): The application entry point delegates parameter parsing to the CLI class, which populates the Config structure from command-line arguments or configuration files.
2.  Workload Ingestion (src/workloads): A BaseWorkload subclass is instantiated based on the configuration state. This instance generates initial arrival events.
3.  Event Loop (src/core/Simulator.cpp, src/core/EventLoop.cpp): A priority queue structured as a min-heap manages simulation time progression. The EventLoop processes ArrivalEvents by assigning incoming tasks to specified Process queues. For SchedulingEvents, the EventLoop requests execution decisions from the designated BaseScheduler.
4.  Scheduling Logic (src/schedulers): The active scheduler processes state data via the choose method. This method calculates metric scores across all active processes and returns a Decision structure containing the target identifier and related metadata.
5.  Metrics and Logging (src/logging): The OnlineMetrics class calculates Lyapunov drift at discrete intervals. The OfflineMetrics class computes latency distributions and fairness indices following simulation completion. The StatisticsDatabase class serializes these metrics to storage.

## Core Class Responsibilities

*   EventQueue: Priority queue for sorting discrete events chronologically.
*   Process: Represents a distinct execution queue and tracks local arrival and service rates.
*   BaseScheduler: Abstract base class specifying the interface for scoring logic and process selection.
*   BaseWorkload: Abstract base class defining the interface for stochastic distribution generation and trace ingestion.
*   StatisticsDatabase: Singleton implementation for collecting and flushing simulation metrics.

## Adding a Custom Scheduler

1.  Create MyScheduler.hpp and MyScheduler.cpp in src/schedulers.
2.  Inherit from BaseScheduler and override the choose method.

```cpp
#pragma once
#include "BaseScheduler.hpp"

class MyScheduler : public BaseScheduler {
public:
    Decision choose(SchedulerContext& ctx) override;
};
```

3.  Implement the scoring logic using the provided SchedulerContext to inspect queue states.
4.  Register the new scheduler in CLI.cpp to map command-line arguments to the class instance.
5.  Add the source file to the EMBI_SCHEDULER_SOURCES variable in CMakeLists.txt.

## Performance and Memory Management

*   Binary Logging: Standard comma-separated value logging requires continuous string allocation. For extended simulations, enable binary logging to utilize packed binary formatting, reducing input/output bottleneck overhead.
*   Trace Loading: The TraceLoader class streams file segments sequentially from non-volatile storage. Loading multi-gigabyte traces into system memory will result in memory allocation failures.
*   Cache Locality: Within the choose method, iterate over process collections sequentially to maximize cache line utilization. Avoid non-contiguous memory access patterns within critical execution paths.

## Debugging Directives

*   Time Drift: Convert floating-point stochastic time distributions to discrete integer ticks utilizing the provided conversion utilities to prevent cumulative rounding errors.
*   RoundRobin Deadlocks: Ensure the internal pointer advances sequentially when encountering empty process queues to prevent infinite loops.
*   Memory Leaks: Ensure EventQueue manages object lifetimes correctly when transferring objects into the priority queue.
