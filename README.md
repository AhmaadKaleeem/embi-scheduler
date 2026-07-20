# EMBI Scheduler

> **EMBI: Predictive OS scheduler derived from minimizing Lyapunov drift of quadratic contention. Reduces tail latency in lock-heavy workloads. C++ simulator and Linux sched_ext prototype.**

> [!WARNING]
> **Research in Progress**: This repository contains active research code. APIs, algorithms, and results are subject to change without notice.

## Overview
EMBI (Estimated Marginal Blocking Impact) is a novel predictive OS scheduler designed to minimize the one-step Lyapunov drift of a quadratic contention potential. It aims to significantly reduce tail latency, particularly in lock-heavy and highly contended workloads.

## Mathematical Foundation
The core scheduling policy of EMBI is derived from minimizing the drift of a Lyapunov function representing system contention.

The scheduling decision $i^*$ is made by maximizing the following metric:

$$ i^* = \arg\max_i \left( Q_i \times (2\mu_i + 2\lambda_i - M) \right)^+ $$

Where:
- $Q_i$: Queue length or wait time for process $i$
- $\mu_i$: Estimated service rate
- $\lambda_i$: Estimated arrival rate
- $M$: Capacity bound factor
- $(\cdot)^+$: Clipped to non-negative values

## Architecture

```text
+-------------------------------------------------------------+
|                      EMBI Scheduler Repo                    |
|                                                             |
|  +-----------------------+       +-----------------------+  |
|  | C++ Discrete-Event    |       | Linux sched_ext       |  |
|  | Simulator (Current)   |       | Prototype (Future)    |  |
|  |                       |       |                       |  |
|  | - Event Queue         |       | - eBPF Programs       |  |
|  | - Schedulers (EMBI,   |       | - BPF Maps            |  |
|  |   MaxWeight, cmu...)  |       | - User-space agent    |  |
|  | - Workloads (Poisson, |       |                       |  |
|  |   Bursty, Trace...)   |       |                       |  |
|  +-----------|-----------+       +-----------------------+  |
|              |                                              |
|              v                                              |
|  +-----------------------+                                  |
|  | Python Analysis /     |                                  |
|  | Visualization Scripts |                                  |
|  +-----------------------+                                  |
+-------------------------------------------------------------+
```

## Quick Start

### Build the Simulator
```bash
cd Event-driven-simulator-C++
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

### Run an Experiment
```bash
./build/embi_sim --scheduler embi --workload bursty --ticks 1000000 --output ../results/run0
```

### Visualize
```bash
python3 scripts/compare_schedulers.py --results ../results/run0
```

## Results

| Scheduler | Workload | p50 Latency | p99 Latency | Throughput |
|-----------|----------|-------------|-------------|------------|
| EMBI      | Bursty   | TBD         | TBD         | TBD        |
| MaxWeight | Bursty   | TBD         | TBD         | TBD        |
| FCFS      | Bursty   | TBD         | TBD         | TBD        |
| cμ        | Bursty   | TBD         | TBD         | TBD        |
| RR        | Bursty   | TBD         | TBD         | TBD        |

*(Results are actively being collected)*

## Citation
If you use EMBI in your research, please cite our upcoming paper:
```bibtex
@misc{embi2026,
  title={EMBI: Predictive OS Scheduling via Lyapunov Drift Minimization},
  author={Ahmaad Kaleem},
  year={2026},
  note={In preparation}
}
```

## Contributing
Please refer to [CONTRIBUTING.md](CONTRIBUTING.md) for details on our code of conduct and the process for submitting pull requests.
