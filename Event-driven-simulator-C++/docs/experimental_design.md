# Experimental Design & Evaluation Methodology

This document outlines the evaluation strategy for the EMBI scheduling framework to ensure the results are robust enough for a systems conference submission (e.g., USENIX ATC, EuroSys, OSDI, SOSP, ASPLOS).

## 1. Core Hypothesis

Before running any experiment, the overarching hypothesis must be defined:

> **Hypothesis:** EMBI achieves lower queue instability and lower starvation than existing scheduling policies while maintaining comparable throughput under dynamic and overloaded workloads.

## 2. Baseline Suite

EMBI is evaluated against a comprehensive suite of established scheduling algorithms:

| Scheduler           | Purpose                          |
| ------------------- | -------------------------------- |
| FCFS                | Simplest baseline                |
| Round Robin         | Time-sharing baseline            |
| SJF                 | Throughput/Latency-oriented baseline |
| CFS                 | Linux-inspired fairness baseline |
| MaxWeight           | Queueing-theory baseline         |
| EMBI                | Proposed scheduler               |
| Hybrid EMBI         | Noise-resilient proposed scheduler |

## 3. Metrics

To provide a complete picture of scheduling behavior, the following metrics are collected per experiment:
* **Throughput**: Jobs completed per tick.
* **Waiting Time**: Mean, Median, P95 (tail latency), P99 (starvation), and Maximum.
* **Queue Length**: Average, Maximum, and Distribution.
* **Lyapunov Potential ($V = \sum Q_i^2$)**: Quantifies queue instability.
* **Lyapunov Drift ($\Delta V$)**: A negative long-term drift indicates queue stability.
* **Starvation**: Average and maximum ticks since last service.
* **CPU Utilization**: Fraction of ticks the CPU was active.
* **Jain's Fairness Index**: $J=\frac{\left(\sum x_i\right)^2}{n\sum x_i^2}$, where values close to 1 indicate perfectly fair allocation.

## 4. Workload Matrices (Experiments 1–8)

1. **Light Load ($\lambda < \mu$)**: Verifies EMBI does not add unnecessary overhead when uncongested.
2. **Heavy Load ($\lambda \approx \mu$)**: Tests scheduling decisions under standard congestion.
3. **Overloaded System ($\lambda > \mu$)**: Tests graceful degradation; no scheduler can keep queues bounded forever, but Lyapunov-based methods should excel here.
4. **Bursty Arrivals**: Tests how quickly each scheduler adapts to sudden spikes.
5. **Changing Arrival Rate**: Tests adaptability over long epochs (e.g., $\lambda = 0.5 \rightarrow 2.0 \rightarrow 0.8$).
6. **Lock Contention**: Tests behavior under synchronization delays (waiting time, queue buildup).
7. **Long Jobs vs Short Jobs**: Tests starvation of heavy jobs vs latency of short jobs.
8. **Adversarial Workload**: Alternating bursts designed to confuse greedy schedulers to test for oscillation.

## 5. Statistical Rigor

* Never rely on a single run. Use at least **30 random seeds** per configuration.
* Report Mean, Standard Deviation, and 95% Confidence Intervals.
* Use Bootstrap confidence intervals and Wilcoxon signed-rank tests for paired comparisons.

## 6. The "Adaptive Workload Transition" (Gold Standard)

The single most critical experiment for an adaptive scheduler is a sharp transition:
* Start with a stable load.
* Increase the arrival rate sharply into extreme congestion.
* Return to a stable load.
* Plot queue lengths, Lyapunov potential, throughput, and waiting time over time.

This directly proves whether EMBI truly *adapts* to changing conditions or merely performs well under a fixed workload.
