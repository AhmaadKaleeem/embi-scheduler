# EMBI CPU Scheduling Simulator

A discrete-event CPU scheduling simulator developed to evaluate scheduling policies. It implements five scheduling algorithms and four stochastic workload models. The software provides an experimental platform for analyzing stochastic scheduling systems and calculating Lyapunov stability bounds.

## Project Overview

### Research Motivation

Cloud computing environments process workload distributions exhibiting heavy-tailed characteristics. Standard scheduling algorithms frequently result in latency degradation and queue starvation under these bursty conditions. The EMBI simulator provides a C++ framework to mathematically model queue stability and measure latency parameters of the EMBI policy relative to MaxWeight and cμ baselines.

### Supported Schedulers

*   EMBI: Calculates priority using bounded queue and service rate differentials.
*   EMBI (unclipped): Standard EMBI calculation omitting boundary constraints.
*   MaxWeight: Prioritizes processes based on the product of queue length and service rate.
*   cμ: Prioritizes based exclusively on strict service rate.
*   Round Robin: Cyclic allocation mechanism.
*   FCFS: Earliest first-arrival time execution.

### Supported Workloads

*   Uniform: Uniform distribution parameterized by lower and upper bounds.
*   Poisson: Exponential distribution parameterized by arrival rate.
*   Bursty: Markov-modulated two-state continuous-time model.
*   Heavy-tail: Pareto distribution characterized by infinite variance.
*   Trace: Deterministic replay from formatted comma-separated value datasets.

## Repository Structure

```text
.
├── CMakeLists.txt
├── docs
├── examples
├── scripts
├── src
│   ├── core
│   ├── logging
│   ├── schedulers
│   ├── trace
│   ├── utils
│   └── workloads
└── tests
```

## Compilation Requirements

*   CMake version 3.20 or newer
*   Compiler supporting C++17
*   Python version 3.8 or newer for script execution
*   GoogleTest and nlohmann/json libraries (resolved automatically during configuration)

## Build Instructions

Execute the following commands to compile the simulator with release optimizations:

```bash
cmake -B build_release -DCMAKE_BUILD_TYPE=Release
cmake --build build_release --config Release
```

Execute the test suite to verify compilation:

```bash
cd build_release
ctest --output-on-failure
```

## Simulator Execution

The simulator provides a command-line interface.

### Standard Execution

Execute the EMBI scheduler over 1,000,000 ticks with a Poisson workload distribution:

```bash
./build_release/bin/embi_sim \
    --scheduler embi \
    --workload poisson \
    --ticks 1000000 \
    --num-processes 64 \
    --arrival-rate 0.5 \
    --seed 42 \
    --output results/poisson_test
```

### Parameter Sweeps

Execute predefined parameter permutations utilizing configuration files:

```bash
./build_release/bin/embi_sim --experiment examples/full_sweep.json
```

### Trace-Driven Execution

Replay empirical traces using the trace ingestion framework. Note: Raw datasets must first be normalized to the canonical structure using the data pipeline.

```bash
# 1. Normalize dataset schemas
./prepare_traces.sh

# 2. Execute simulation using the canonical output
./build_release/bin/embi_sim \
    --scheduler maxweight \
    --workload trace \
    --trace scripts/trace_profiler/output_reports/alibaba/canonical.csv \
    --output results/alibaba_eval
```

## Output Metrics

Outputs are written to the specified output directory.

*   run.csv: Tick-level state variables including queue lengths and arrival counts.
*   summary.txt: Aggregate statistics detailing waiting times and Lyapunov drift calculations.
*   summary.json: Serialized JSON metrics required for Python visualization scripts.

Key metrics tracked include Lyapunov potential, latency percentiles, Jain's Fairness Index, and systemic throughput.

## Documentation Reference

*   Developer Guide: Defines architecture, custom scheduler implementation, and memory management.
*   Reproducibility Guide: Specifies commands and configurations required to replicate experimental results.
