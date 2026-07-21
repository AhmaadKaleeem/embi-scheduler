# EMBI Enhancements & Baseline Schedulers Implementation Plan

This plan addresses all 4 mandatory changes requested to evaluate the effectiveness of the EMBI scheduler and extract a formal "Contribution Statement" for the paper.

## Proposed Changes

### 1. Fix the Broken Lyapunov Metric
We will track the Time-Averaged Lyapunov potential, `V_avg`, over the entire simulation rather than only sampling the final tick `V(final)`.
- **`EventLoop.cpp`**: Accumulate `sum_lyapunov_v += online_metrics.lyapunovV()` at every simulation tick.
- **`OfflineMetrics.hpp/cpp`**: Receive the accumulated sum and compute `avg_lyapunov_v = sum_lyapunov_v / total_ticks`.
- **`SummaryWriter.cpp` & `main.cpp`**: Update the reporting interface to display `V_avg`.

### 2. Add a Tunable Fairness Constraint (The "Magic Knob" $\beta$)
We will introduce a dynamic fairness penalty into the EMBI loss function to force it to balance exploration vs. exploitation: `Loss = M + \beta * (1 / Jain_Fairness)`.
- **`Config.hpp` & `CliParser.cpp`**: Introduce `double fairness_beta` and CLI flag `--fairness-beta`.
- **`Scheduler.hpp`**: Add `current_jain_fairness` to the `SchedulerContext` struct.
- **`EventLoop.cpp`**: Compute the instantaneous Jain Fairness across all CPU shares and pass it to the scheduler context.
- **`EMBIScheduler.cpp`**: Dynamically compute the modified loss function using the provided $\beta$ parameter.

### 3. Implement Adaptive Active Probing Magnitude ($\delta$)
To address the starvation in Google vs. Alibaba, we will replace fixed magnitude variables with an adaptive probing offset proportional to the expected service time of the jobs in the queue ($\delta \times \text{Expected Service Time}$).
- **`Config.hpp` & `CliParser.cpp`**: Introduce `double probe_delta` and CLI flag `--probe-delta`.
- **`EMBIScheduler.cpp`**: Adapt the causal influence calculation to scale by $\delta \times (1.0 / \hat{\mu})$, ensuring perturbations are scaled properly for both microsecond-level and hour-level trace jobs.

### 4. Implement Mandatory Baseline Schedulers
We will introduce two new industry-standard schedulers to act as competitive baselines against EMBI.
- **[NEW] `src/schedulers/SJFScheduler.hpp/cpp`**: Shortest Job First (implemented as Shortest *Expected* Job First based on $\hat{\mu}$). This establishes the absolute minimum theoretical latency boundary.
- **[NEW] `src/schedulers/CFSScheduler.hpp/cpp`**: Linux Completely Fair Scheduler. This establishes the real-world virtual runtime (vruntime) fairness boundary.
- **`Simulator.cpp` & `Config.hpp`**: Register `sjf` and `cfs` as valid schedulers in the factory pattern.

### 5. Create the "Golden" Experiment Matrix Pipeline
- **[NEW] `run_sweep.ps1`**: A dedicated evaluation pipeline that loops through all permutations of the parameters ($\beta \in [0.0, 1.0]$, $\delta \in [0.01, 0.1]$, and Schedulers `rr`, `sjf`, `cfs`, `embi`) across the 3 canonical traces. This will automatically spit out the CSV dataset needed for the Pareto-frontier graph.

## Open Questions

> [!IMPORTANT]
> - Do you want the `run_sweep.ps1` script to plot the Pareto Frontier automatically using a python visualization script, or just dump the CSV matrix for you to plot externally?

## Verification Plan
1. Recompile the simulator using `build_simulator.ps1`.
2. Run single instances of `sjf` and `cfs` to verify they compile and route correctly.
3. Validate that `V_avg` tracks correctly across high-contention scenarios.
4. Execute `run_sweep.ps1` to ensure all configurations run properly without crashing.
