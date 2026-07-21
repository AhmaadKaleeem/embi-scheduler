# Research Reproducibility Guide

This guide specifies the procedures necessary to replicate the experiments, Lyapunov stability validations, and latency measurements documented in the research paper.

## 1. Artifact Packaging and Build Environment

The codebase remains fixed at the designated release commit. Execution yields identical outputs provided the specified random seeds and configurations are utilized.

*   Dependencies: CMake 3.20 or newer, GCC 11 or newer.
*   Build Mode: The binary must be compiled in Release mode to replicate execution time benchmarks.

```bash
cmake -B build_release -DCMAKE_BUILD_TYPE=Release
cmake --build build_release
```

## 2. Experiment Reproduction

### Experiment 1: Baseline Poisson Workload (Figure 2)

This experiment evaluates the EMBI scheduler relative to MaxWeight under a Poisson arrival distribution.

```bash
./build_release/bin/embi_sim --experiment examples/quick_demo.json --seed 42 --output results/fig2_poisson
```

Expected Output: Execution generates summary.txt, detailing EMBI bounded queue growth calculations.

### Experiment 2: Parameter Sweep (Figure 3 and Figure 4)

This script varies the arrival rate across 6 schedulers, measuring Jain's Fairness Index and P99 tail latency.

```bash
./build_release/bin/embi_sim --experiment examples/full_sweep.json
```

Note: This configuration processes 1080 distinct parameter permutations utilizing thread pool parallelization. Execution requires approximately 10 minutes on a 16 core processor.

### Experiment 3: Lock Contention Analysis (Table 1)

Evaluates scheduler performance during service rate degradation induced by simulated lock contention.

```bash
./build_release/bin/embi_sim --experiment examples/lock_contention_sweep.json
```

## 3. Dataset Preparation

Prior to trace-driven execution, empirical datasets (e.g., Azure, Alibaba, Google cluster traces) must be standardized into a canonical format to guarantee uniform ingestion by the C++ simulator.

1.  Ensure raw datasets are positioned in the `Dataset/` directory.
2.  Execute the trace preparation pipeline:

```bash
./prepare_traces.sh
```

The pipeline executes the following sequence:
*   Discovery: Identifies raw datasets.
*   Schema Inference: Analyzes delimiter types and column data types.
*   Quality Check: Identifies anomalous values and sparsity.
*   Semantic Mapping: Translates dataset-specific schema elements into the canonical execution format.

Canonical outputs and processing manifests are deposited in `scripts/trace_profiler/output_reports/`.

## 4. Metric Plotting

Following experiment execution, use the provided Python scripts to convert the summary.json outputs into graphical representations.

1.  Install Python dependencies:

```bash
pip install matplotlib pandas seaborn
```

2.  Execute visualization scripts:

```bash
python scripts/visualization/plot_latency.py --input results/fig2_poisson/summary.json --output paper/fig2.pdf
python scripts/visualization/plot_pareto_curve.py --input output/summary.json --output paper/fig3.pdf
```

## 5. Verification Directives

If reproduction results deviate from published values:

1.  Seed Consistency: Verify the seed parameter was applied exactly as specified.
2.  Compiler Flags: Verify optimization flags were applied during compilation. Debug builds alter execution timing and impact multi-threaded stochastic generation.
3.  Dataset Checksums: When executing trace experiments, verify the SHA256 hashes of the datasets match the provided manifest located in dataset/manifest.json.
