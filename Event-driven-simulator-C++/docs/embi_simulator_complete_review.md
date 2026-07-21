# EMBI Simulator Complete Architecture & Methodology Review

The simulator implements a high-fidelity **discrete-event simulation (DES)** of a CPU scheduler. It does not use OS-level threads or `sleep()` calls. Instead, it advances a virtual clock from event to event (Arrival, Service, metrics sampling).

> [!IMPORTANT]
> **Single CPU Assumption:** This simulator models a single server execution engine. This paper studies scheduling policy independent of multicore effects (NUMA, migration).

---

## A. Experiment Configurations & Baselines

By default, experiments are defined via `config.default.yaml` and expanded via the `ExperimentConfig` matrix generator. The default baseline parameters across experiments are:

* **Runtime (Ticks)**: `1,000,000` (provides substantial steady-state measurement)
* **Warmup / Cooldown**: The system pre-allocates jobs and initializes EWMA estimators with "ground truth" to eliminate cold-start noise. No explicit cooldown is needed as metrics are time-averaged.
* **Process Count ($N$)**: `16` (scalable to `64+`). Queue count maps 1:1 to process count.
* **Arrival Rate ($\lambda$)**: `0.5` jobs/tick per process.
* **Service Rate ($\mu$)**: `1.0` jobs/tick.
* **Lock Hold Time**: Exponential distribution with mean `5.0` ticks.

---

## B. Hybrid Scheduler Implementation (Exact Logic)

The Hybrid EMBI uses an instantaneous confidence gate rather than historical variance. The exact C++ logic is:

```cpp
// 1. Compute embi_scores and find best choice (d_embi)
computeDecisionDiagnostics(embi_scores, d_embi); 

// 2. Confidence-Gated Fallback Logic
if (d_embi.score_delta > eta_max_) {
    // High confidence: Signal dominates noise. Use EMBI.
    return d_embi; 
} else {
    // Low confidence: Noise dominates signal. Fall back to MaxWeight.
    // ... compute mw_scores = mu_hat * 2.0 * queue_length ...
    return d_mw;
}
```
*Note*: `score_delta` is the absolute difference between the 1st place and 2nd place EMBI scores.

---

## C. EWMA Implementation ($\hat{\lambda}, \hat{\mu}$)

* **Smoothing Factors**: $\alpha = 0.1$ (arrival), $\beta = 0.1$ (service).
* **Initialization (Simulation Convenience)**: $\hat{\lambda}$ and $\hat{\mu}$ are aggressively pre-initialized to the configuration's true rates. We initialize the EWMA to the true rate only to eliminate transient estimator bias. Sensitivity experiments with cold initialization are provided.
* **First Arrival Handling**: When the absolute first job arrives, `first_arrival_time` is recorded, but the EWMA is *not* updated (because inter-arrival time is undefined). The EWMA begins updating on the *second* arrival.

---

## D. Lock Model Mechanics

* **Probability of Request**: Poisson process defined by `lock_request_rate` (default 0.3 requests per process per tick).
* **Service Interruption**: When a process acquires a lock, it receives a `required_cpu` quota. It must be scheduled for that many CPU ticks before the lock releases.
* **Blocking & Synchronization Debt**: If a process requests a lock that is already held, it is blocked. **Crucially**, while blocked, the *current owner's* mathematical **Synchronization Debt ($D_i$)** increments. We use the score $Q_i + D_i$. We are not pretending the queue literally grows. We are introducing a mathematical congestion state that models the fluid "CPU debt" the owner is accumulating by holding up the system, allowing EMBI to naturally react to contention.

---

## E. Metrics Formulas

Metrics are strictly computed in `OfflineMetrics.cpp` to prevent runtime observer effects:
* **Tail Latency (P95/P99)**: The `wait_times_` vector is fully sorted. Exact values are extracted via `sorted[N * 0.99]`. No HDR histograms or binning approximations are used.
* **Mean/Median**: Computed exactly from the sorted vector.
* **Throughput**: `completed_jobs / total_ticks`.
* **Utilization**: `(total_ticks - idle_ticks) / total_ticks`.
* **Starvation**: Max consecutive ticks a process was bypassed while `queue_length > 0`.
* **Fairness**: Jain's Fairness Index: $J=\frac{\left(\sum x_i\right)^2}{n\sum x_i^2}$ (where $x$ is CPU share).
* **Lyapunov Drift**: $\Delta V = V(t) - V(t-1)$.

---

## F. Trace Mapping & Preprocessing

The simulator uses `build_canonical.py` to standardize extreme real-world traces (Alibaba, Azure, Google) into a unified 10-column canonical schema: `[tick, trace_id, rpc_id, source, dest, container, latency, flags, type, priority]`.

* **Alibaba**: 
  * `tick` $\leftarrow$ Timestamp
  * `trace_id`, `source`, `dest`, `container` $\leftarrow$ MD5 hashed strings
  * `latency` $\leftarrow$ `float(row[4]) * 1000` (scaled to ticks)
* **Azure**:
  * `tick` $\leftarrow$ `(end_timestamp - duration) * 1000`
  * `latency` $\leftarrow$ `duration * 1000`
* **Google**:
  * `tick` $\leftarrow$ `time // 1000000`
  * `latency` $\leftarrow$ `cpus * 1000`
  * `priority` $\leftarrow$ Extracted directly from JSON.

*All traces are chronologically sorted and zero-indexed to $t=0$ before simulation.*

---

## G. Output Pipeline

1. **Raw Dumps**: The C++ engine dumps `summary.json` (aggregated statistics) and CSV files (tick-by-tick logs).
2. **JSON Schema**: Contains nested blocks for `latency`, `throughput`, `fairness`, `stability`, and `scheduler_diagnostics`.
3. **Python Aggregation**: Scripts like `generate_summaries.py` ingest the JSON/CSV files across all seeds, aggregating means and standard deviations using `pandas`.
4. **Figure Generation**: Data is fed into `matplotlib` to generate time-series plots of Queue Lengths, Lyapunov Drift, and CDFs of Wait Times.

---

## H. Benchmark & Statistical Protocol

* **Repetitions**: Every configuration is swept across multiple independent random seeds (default 30 for production runs) via the `ExperimentConfig` matrix.
* **Seed Generation**: Deterministic pseudo-randomness using C++11 `std::mt19937_64` (Mersenne Twister), ensuring bit-for-bit reproducibility.
* **Statistical Significance**: The C++ simulator isolates data generation. Normality testing, Bootstrap confidence intervals, and Wilcoxon signed-rank tests are deferred to the Python `scipy.stats` post-processing pipeline to ensure publication-quality statistical validation.

---

## 1-10. Core Architecture Summary
*(For completeness, the foundational architectural rules)*

1. **Preemptive Scheduling**: `choose()` runs every single tick unconditionally.
2. **Mechanism vs Policy**: `EventLoop` handles time and state; `choose()` is a pure policy function operating on an immutable `SchedulerContext`.
3. **Event Causality**: Handled strictly via priority heap: `Arrival < Lock < Schedule < Service < Metrics`.
4. **Queue Architecture**: Single server, no migration, isolated FIFO deques per process.
5. **Complexity**: $O(\log E + N)$ per tick.
