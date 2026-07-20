#!/bin/bash
# Parameter sweep comparing multiple schedulers

set -e

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SIM_DIR="$REPO_ROOT/Event-driven-simulator-C++"
BUILD_DIR="$SIM_DIR/build_release"
OUTPUT_DIR="$REPO_ROOT/results/sweep_comparison"

echo "Building simulator..."
cmake -B "$BUILD_DIR" -S "$SIM_DIR" -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD_DIR" -j$(nproc)

echo "Running scheduler sweep..."
mkdir -p "$OUTPUT_DIR"

# Using the full sweep config defined in the simulator directory
SWEEP_CONFIG="$SIM_DIR/examples/full_sweep.json"

if [ ! -f "$SWEEP_CONFIG" ]; then
    echo "Error: Sweep config not found at $SWEEP_CONFIG"
    exit 1
fi

"$BUILD_DIR/embi_sim" --experiment "$SWEEP_CONFIG" --output "$OUTPUT_DIR"

echo "Sweep complete! Results saved to $OUTPUT_DIR"
