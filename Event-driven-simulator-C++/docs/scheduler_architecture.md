# Scheduler Architecture & Event Flow

This document defines the simulation model, scheduling architecture, and system invariants. It is structured to support a high-quality systems conference methodology section.

## 1. Simulation Model & Abstractions

The simulator models a **single-server queueing system** operating in discrete time. Each tick corresponds to one unit of processor service, and at most one job may receive service during each tick. 

To isolate and mathematically evaluate scheduling policies, the simulator intentionally abstracts away hardware and kernel-level artifacts. Specifically, it omits:
* Context-switch cost
* Caches, TLBs, and branch prediction
* NUMA and CPU affinity
* Interrupt latency and kernel execution overhead
* Multicore scheduling (for the baseline models)

### Determinism Guarantee
Given an identical workload trace, random seed, and configuration, the simulator is **fully deterministic**. Event ordering is uniquely defined by `(tick, EventType)`, and scheduler policies operate on immutable context snapshots. This guarantees perfectly reproducible experiments.

## 2. Policy vs. Mechanism

The simulator strictly enforces the classical operating systems separation between **mechanism** and **policy**. 

The `EventLoop` implements the scheduling mechanism by advancing simulation time, processing events, and executing service quanta. Scheduling algorithms implement only the policy through `Scheduler::choose(const SchedulerContext&)`, which is a pure decision function returning the selected process identifier. The scheduler has no authority to mutate simulator state directly.

### Formalized Decision Function
A scheduling decision is mathematically formalized as:
$$a_t = \pi(s_t)$$
where:
* $\mathcal{S}$ = scheduler state space (`SchedulerContext`)
* $\mathcal{A}$ = process IDs plus idle
* $\pi : \mathcal{S} \rightarrow \mathcal{A}$ is the scheduling policy.

## 3. The Tick Pipeline & Event Ordering

At every tick, events are popped from a min-heap `EventQueue`. To preserve strict causality and prevent off-by-one accounting errors, events firing at the exact same tick are resolved using their `EventType` enumeration priority:

```text
Arrival (0)  →  LockAcquire (1)  →  LockRelease (2)  →  Schedule (3)  →  Service (4)  →  Metrics (5)
```

**Why Metrics Come Last:**
If `Metrics` fired before `Service`, throughput, queue lengths, and Lyapunov drift would reflect the system state *before* work was completed during that tick, introducing a permanent one-tick lag in accounting.

### Preemptive Scheduling Model
Unlike Linux, which schedules on demand following wakeups, preemption points, and explicit rescheduling events, this simulator invokes `choose()` **once per simulation tick**. This models an idealized fully preemptive scheduler with fixed-length service quanta, simplifying policy comparison while avoiding implementation-specific timing artifacts.

## 4. Architecture Diagram

```text
             Workload Generator
                     │
                     ▼
              Event Generator
                     │
                     ▼
              Priority Event Queue
                     │
                     ▼
               EventLoop::run()
                     │
      ┌──────────────┼──────────────┐
      │              │              │
      ▼              ▼              ▼
  Arrival       Lock Events     Schedule
                                     │
                                     ▼
                            SchedulerContext
                                     │
                                     ▼
                               Scheduler
                              choose(ctx)
                                     │
                                     ▼
                              Selected PID
                                     │
                                     ▼
                              Service Event
                                     │
                                     ▼
                              Metrics Update
```

## 5. System Invariants
These invariants mathematically constrain the simulation and simplify correctness arguments:
1. `queue_length ≥ 0` across all processes at all times.
2. Only one `ServiceEvent` executes per tick (single-server model).
3. Metrics are recorded strictly after service completes for that tick.
4. `choose()` is a pure function and never mutates simulator state.
5. `SchedulerContext` is an immutable snapshot of the system at time $t$.

## 6. The `SchedulerContext` Snapshot

At each scheduling instant, the simulator constructs an **immutable snapshot** of the current system state (`SchedulerContext`). Scheduler implementations operate exclusively on this snapshot.

### Instantaneous State
* `current_tick`: The absolute simulation time.
* `previous_decision`: PID of the process executed in the preceding tick.
* `queue_length`: Number of queued/active jobs for each process.
* `holArrivalTime()`: Arrival tick of the Head-of-Line job.
* `lock_state`: Mutex hold status and required CPU time for completion.

### Estimated State
* `lambda_hat` ($\hat{\lambda}$): EWMA of arrival rate.
* `mu_hat` ($\hat{\mu}$): EWMA of service rate.

### Historical State
* `ticks_since_last_service`: Starvation duration counter.
* `total_waiting_time`: Cumulative job waiting time per process.

### Global State
* `lyapunov_v`: Instantaneous Lyapunov potential ($V(t)$).
* `lyapunov_drift`: Drift from previous tick ($\Delta V$).
* `throughput` & `utilization`: System efficiency metrics.

## 7. Asymptotic Complexity

The simulation engine is optimized for scale (handling $10^6+$ ticks and massive datasets):
* **Event Queue**: `push()` is $O(\log E)$, `pop()` is $O(\log E)$.
* **Scheduling (FCFS, SJF, EMBI)**: $O(N)$ where $N$ is the number of processes.
* **Metrics Snapshot**: $O(N)$.
* **Overall Time Complexity**: $O(\log E + N)$ per tick.
