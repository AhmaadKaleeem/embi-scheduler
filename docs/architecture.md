# EMBI Architecture

The EMBI Scheduler repository is divided into two primary components: the C++ Event-driven Simulator and the upcoming Linux sched_ext prototype.

## 1. C++ Event-driven Simulator
Located in `Event-driven-simulator-C++/`, this simulator is designed for rapid prototyping and rigorous mathematical validation of the EMBI policy against classical schedulers.

### Core Components
- **Event Loop**: A highly optimized min-heap based event queue processing arrivals, completions, and scheduling ticks.
- **Process Model**: Tracks queue lengths, waiting times, and computes Exponentially Weighted Moving Averages (EWMA) of arrival ($\lambda$) and service ($\mu$) rates online.
- **Scheduler Interface**: An abstract base `BaseScheduler` implementing the `choose()` method. Schedulers receive a `SchedulerContext` snapshot.
- **Workload Generators**: Pluggable models including Uniform, Poisson, Markov-modulated Bursty, Heavy-tail (Pareto), and Trace-driven replays.
- **Metrics Engine**: Computes online (Lyapunov drift, throughput) and offline (P99 tail latency, Jain Fairness Index) metrics.

## 2. Linux sched_ext Prototype (Future)
The production implementation will leverage the Linux kernel's `sched_ext` framework to inject the EMBI policy dynamically via eBPF.

### Planned Architecture
- **eBPF Kernel Component**: Enqueues and dequeues tasks, updating EWMA stats in BPF maps. Computes the EMBI score for fast-path decisions.
- **User-space Agent**: Periodically adjusts the capacity bound factor $M$ and logs detailed metrics for analysis.
