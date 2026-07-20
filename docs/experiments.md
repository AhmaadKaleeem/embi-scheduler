# Running Experiments

This guide explains how to configure and execute experiments in the EMBI simulator.

## Basic Single Run
To run a single simulation instance, use the CLI flags to define the environment:

```bash
cd Event-driven-simulator-C++
./build/embi_sim --scheduler embi \
                 --workload poisson \
                 --ticks 5000000 \
                 --num-processes 32 \
                 --arrival-rate 0.8 \
                 --M 15.0 \
                 --output results/test_run
```

## Parameter Sweeps
For comprehensive evaluation, the simulator supports JSON-based experiment configurations to run parameter sweeps across multiple dimensions (e.g., varying arrival rates or $M$ bounds).

1. Define an experiment config (see `examples/run_sweep.sh` for usage).
2. Execute the sweep:
```bash
./build/embi_sim --experiment path/to/sweep_config.json
```

## Output Artifacts
Each run or sweep generates data in the output directory:
- `run.csv`: High-resolution per-tick metrics (Lyapunov drift, queue sizes).
- `summary.txt`: Human-readable latency percentiles and fairness scores.
- `summary.json`: Machine-readable results for automated Python plotting.

## Visualization
Use the Python scripts to generate comparison plots:
```bash
python scripts/compare_schedulers.py --data results/sweep_output/summary.json --out figs/
```
