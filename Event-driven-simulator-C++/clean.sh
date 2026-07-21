#!/bin/bash

echo "======================================"
echo " Cleaning EMBI Simulator Workspace"
echo "======================================"

DIRS=(
    "build_release"
    "results"
    "scripts/trace_profiler/output_reports"
)

for dir in "${DIRS[@]}"; do
    if [ -d "$dir" ]; then
        echo "Removing $dir..."
        rm -rf "$dir"
    fi
done

# Clean __pycache__ directories
find . -type d -name "__pycache__" -exec rm -rf {} + 2>/dev/null

echo "======================================"
echo " Workspace is clean!"
echo "======================================"
