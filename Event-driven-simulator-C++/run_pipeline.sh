#!/usr/bin/env bash
# run_pipeline.sh
# End-to-end execution pipeline for EMBI Scheduling Simulator

set -e

# Define directories
PDF_DIR="results/pdf_figures"
TRACE_OUTPUT_DIR="scripts/trace_profiler/output_reports"

echo "======================================"
echo " PHASE 1: SYNTHETIC BENCHMARKS"
echo "======================================"
echo " 2. Building Simulator (Release Mode)"
echo "======================================"
cmake -B build_release -DCMAKE_BUILD_TYPE=Release
cmake --build build_release -j $(nproc 2>/dev/null || echo 4)

if [ -f "./build_release/Release/embi_sim.exe" ]; then
    EMBI_SIM="./build_release/Release/embi_sim.exe"
elif [ -f "./build_release/bin/embi_sim" ]; then
    EMBI_SIM="./build_release/bin/embi_sim"
else
    EMBI_SIM="./build_release/embi_sim"
fi

mkdir -p "$PDF_DIR"

echo "--------------------------------------"
echo " 1.2 Executing 6 Synthetic Experiments"
EXPERIMENTS=(
    "exp1_symmetric.json"
    "exp2_asymmetric.json"
    "exp3_high_contention.json"
    "exp4_long_hold.json"
    "exp5_scalability.json"
    "exp6_confidence_hybrid.json"
)

for exp in "${EXPERIMENTS[@]}"; do
    echo "Running synthetic experiment: $exp..."
    "$EMBI_SIM" --experiment "examples/$exp" --output "results/${exp%.*}"
done

echo "--------------------------------------"
echo " 1.3 Generating Synthetic PDF Graphs"
if command -v python3 &> /dev/null; then
    echo "Plotting latency distributions..."
    python3 scripts/visualization/plot_latency.py --input results/exp1_symmetric/summary.json --output "$PDF_DIR/exp1_latency.pdf" || true
    
    echo "Plotting Pareto curves..."
    python3 scripts/visualization/plot_pareto_curve.py --input results/exp2_asymmetric/summary.json --output "$PDF_DIR/exp2_pareto.pdf" || true
    
    echo "Plotting Fairness..."
    python3 scripts/visualization/plot_fairness.py --input results/exp3_high_contention/summary.json --output "$PDF_DIR/exp3_fairness.pdf" || true
else
    echo "Warning: Python3 not found. Skipping plot generation."
fi

echo ""
echo "======================================"
echo " PHASE 2: TRACE DATA PROFILING & CLEANING"
echo "======================================"
echo " 2.1 Analyzing and Building Real-World Datasets"
chmod +x prepare_traces.sh
./prepare_traces.sh

echo ""
echo "======================================"
echo " PHASE 3: TRACE-DRIVEN BENCHMARKS"
echo "======================================"
echo " 3.1 Executing Simulator on Cleaned Traces Separately"
TRACES=("google" "azure" "alibaba")

for trace_name in "${TRACES[@]}"; do
    TRACE_FILE="${TRACE_OUTPUT_DIR}/${trace_name}/canonical.csv"
    RESULT_DIR="results/trace_eval_${trace_name}"
    
    echo "--------------------------------------"
    if [ -f "$TRACE_FILE" ]; then
        echo "Running trace evaluation for: $trace_name"
        "$EMBI_SIM" --scheduler embi --workload trace --trace "$TRACE_FILE" --output "$RESULT_DIR"
        echo "Saved results to: $RESULT_DIR"
    else
        echo "Skipping $trace_name: Canonical trace file not found at $TRACE_FILE."
    fi
done

echo "======================================"
echo " Pipeline Complete."
echo " Synthetic results available in results/exp*/"
echo " Trace results available in results/trace_eval_*/"
echo " Graphs available in $PDF_DIR/"
echo "======================================"
