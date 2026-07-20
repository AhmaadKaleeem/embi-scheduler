# EMBI CPU Scheduling Simulator

A **research-quality, discrete-event CPU scheduling simulator** implementing five scheduling algorithms across four configurable workload types. Designed as the experimental platform for a research paper on stochastic scheduling and Lyapunov stability analysis.

---

## Algorithms

| Scheduler | Description |
|-----------|-------------|
| **EMBI** | EMBI policy: Q_i × (2μ_i + 2λ_i − M), clipped to non-negative |
| **EMBI (unclipped)** | EMBI without score clipping |
| **MaxWeight** | Maximises Q_i · μ_i across processes |
| **cμ** | Prioritises highest service rate μ_i |
| **Round Robin** | Cyclic allocation, skipping empty queues |
| **FCFS** | Earliest first-arrival time |

## Workloads

| Workload | Description |
|----------|-------------|
| **Uniform** | U(lo, hi) inter-arrival times |
| **Poisson** | Exponential(λ) inter-arrival times |
| **Bursty** | Markov-modulated two-state ON/OFF |
| **Heavy-tail** | Pareto(scale, shape) — infinite variance |
| **Trace** | Replay from CSV file |

## Build

### Prerequisites
- CMake ≥ 3.20
- C++17 compiler (GCC ≥ 11, Clang ≥ 14, MSVC 2022)
- Internet access for first build (FetchContent pulls GoogleTest, nlohmann/json)

### Debug build
```sh
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

### Release build
```sh
cmake -B build_release -DCMAKE_BUILD_TYPE=Release
cmake --build build_release --config Release
```

### ASan / UBSan build
```sh
cmake -B build_asan -DCMAKE_BUILD_TYPE=Asan
cmake --build build_asan
```

### Run tests
```sh
cd build
ctest --output-on-failure
```

## Usage

### Single run
```sh
./embi_sim --scheduler embi \
           --workload poisson \
           --ticks 1000000 \
           --num-processes 64 \
           --arrival-rate 0.5 \
           --M 10.0 \
           --seed 42 \
           --output results/run0
```

### Parameter sweep
```sh
./embi_sim --experiment examples/full_sweep.json
```

### Quick demo
```sh
./embi_sim --experiment examples/quick_demo.json
```

### Load config from file (YAML)
```sh
./embi_sim --config my_config.yaml
```

### All flags
```
--scheduler NAME       embi, embi_unclipped, maxweight, cmu, rr, fcfs
--workload NAME        uniform, poisson, bursty, heavy_tail, trace
--profile NAME         web_server, database, cloud_vm, embedded, real_time, interactive_desktop
--ticks N              Simulation ticks (default: 1000000)
--num-processes N      Number of processes (default: 16)
--seed N               PRNG seed (default: 42)
--arrival-rate F       Mean arrivals/tick (default: 0.5)
--service-rate F       Mean service rate (default: 1.0)
--alpha F              EWMA arrival smoothing (default: 0.1)
--beta F               EWMA service smoothing (default: 0.1)
--M F                  EMBI capacity bound (default: 10.0)
--no-clip              Use unclipped EMBI
--trace FILE           Trace CSV file
--output DIR           Output directory (default: output)
--log-freq N           Log every N ticks (default: 1)
--binary-log           Use packed binary log format
--null-log             Discard log output (benchmarking)
--config FILE          Load YAML or JSON config
--experiment FILE      Run a parameter sweep from JSON
--help / -h            Show help
```

## Output

Each run writes the following to `--output`:

| File | Description |
|------|-------------|
| `run.csv` or `run.bin` | Per-tick per-process log |
| `summary.txt` | Human-readable metrics report |
| `summary.json` | Machine-readable metrics (for Python) |

Experiment sweeps write a combined comparative table:

| File | Description |
|------|-------------|
| `summary.txt` | Ranked comparative table (by Jain Fairness Index) |
| `summary.json` | All run summaries as JSON array |

## Metrics

### Online (updated every tick)
- Lyapunov potential V(t) = Σ Q_i²
- Lyapunov drift ΔV = V(t) − V(t−1)
- Rolling throughput (configurable window)
- CPU utilisation

### Offline (computed after simulation)
- Average, P50, P95, P99, max waiting time
- Average turnaround time
- Jain Fairness Index
- Starvation analysis per process
- Queue length statistics (min, max, mean, median, variance)
- Time to steady state
- Oscillation frequency (zero crossings of ΔV)
- Scheduler decision entropy and score variance

## Project structure

```
src/
  CLI.{hpp,cpp}              ← Argument parser
  main.cpp                   ← Entry point

  core/
    Config.{hpp,cpp}         ← Simulation configuration
    Event.hpp                ← Event types and factories
    EventQueue.{hpp,cpp}     ← Min-heap event queue
    Process.{hpp,cpp}        ← Process model with EWMA
    OnlineMetrics.{hpp,cpp}  ← Per-tick Lyapunov / throughput
    OfflineMetrics.{hpp,cpp} ← Post-simulation histogram statistics
    EventLoop.{hpp,cpp}      ← Discrete-event simulation engine
    Simulator.{hpp,cpp}      ← High-level simulation driver
    Results.hpp              ← Results container
    Experiment.{hpp,cpp}     ← Parameter sweep manager

  schedulers/
    Decision.hpp             ← Decision struct with diagnostics
    SchedulerContext.hpp     ← Context passed to choose()
    BaseScheduler.{hpp,cpp}  ← Abstract scheduler interface
    EMBIScheduler.{hpp,cpp}
    MaxWeightScheduler.{hpp,cpp}
    CmuScheduler.{hpp,cpp}
    RoundRobinScheduler.{hpp,cpp}
    FCFSScheduler.{hpp,cpp}

  workloads/
    BaseWorkload.hpp         ← Abstract workload interface
    UniformWorkload.{hpp,cpp}
    PoissonWorkload.{hpp,cpp}
    BurstyWorkload.{hpp,cpp}
    HeavyTailWorkload.{hpp,cpp}
    TraceLoader.{hpp,cpp}    ← CSV trace replay
    WorkloadProfile.{hpp,cpp}← Named workload profiles

  logging/
    Logger.hpp               ← Abstract logger + LogRecord
    NullLogger.hpp           ← Zero-overhead null sink
    CSVLogger.{hpp,cpp}      ← Buffered CSV output
    BinaryLogger.{hpp,cpp}   ← Packed binary output
    Statistics.{hpp,cpp}     ← Aggregate metric accumulator
    StatisticsDatabase.{hpp,cpp} ← Unified output manager
    SummaryWriter.{hpp,cpp}  ← Comparative reports

  utils/
    Random.{hpp,cpp}         ← MT19937 PRNG wrapper
    Timer.hpp                ← High-resolution wall-clock timer
    FileUtils.{hpp,cpp}      ← Directory/path utilities
    ConfigLoader.{hpp,cpp}   ← YAML/JSON config parser

tests/
  test_config.cpp            ← Config and ExperimentConfig tests
  test_workloads.cpp         ← Workload statistical tests
  test_schedulers.cpp        ← All scheduler unit tests
  test_process.cpp           ← Process queue/EWMA/starvation tests
  test_event_queue.cpp       ← EventQueue ordering tests
  test_random.cpp            ← PRNG tests
  test_metrics_online.cpp    ← OnlineMetrics tests
  test_metrics_offline.cpp   ← OfflineMetrics tests
  test_statistics_db.cpp     ← StatisticsDatabase tests
  test_cli.cpp               ← CLI parser tests
  test_config_loader.cpp     ← YAML/JSON loader tests
  test_experiment.cpp        ← End-to-end integration tests
  test_regression.cpp        ← Reproducibility + invariant tests

examples/
  quick_demo.json            ← 6-run demo experiment
  full_sweep.json            ← 1080-run research sweep
```

## Licence

MIT License — see `LICENSE`.
