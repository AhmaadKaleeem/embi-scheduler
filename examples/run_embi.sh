#!/bin/bash
# Single run of the EMBI scheduler

set -e

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SIM_DIR="$REPO_ROOT/Event-driven-simulator-C++"
BUILD_DIR="$SIM_DIR/build_release"
OUTPUT_DIR="$REPO_ROOT/results/embi_single_run"

echo "Building simulator..."
cmake -B "$BUILD_DIR" -S "$SIM_DIR" -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD_DIR" -j$(nproc)

echo "Running EMBI simulation..."
mkdir -p "$OUTPUT_DIR"

"$BUILD_DIR/embi_sim" \
    --scheduler embi \
    --workload bursty \
    --ticks 2000000 \
    --num-processes 64 \
    --arrival-rate 0.6 \
    --M 12.0 \
    --seed 42 \
    --output "$OUTPUT_DIR"

echo "Simulation complete! Results saved to $OUTPUT_DIR"
cat "$OUTPUT_DIR/summary.txt"
