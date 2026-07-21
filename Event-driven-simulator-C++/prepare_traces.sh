#!/usr/bin/env bash
# prepare_traces.sh
# Trace Data Processing Automation Pipeline

set -e

echo "======================================"
echo " Trace Data Processing Pipeline"
echo "======================================"

# Verify Python 3 is installed
if ! command -v python3 &> /dev/null; then
    echo "Error: Python3 is required but not installed."
    exit 1
fi

# Verify dataset directory exists
DATASET_DIR="../Dataset"
if [ ! -d "$DATASET_DIR" ]; then
    echo "Warning: Expected dataset directory at $DATASET_DIR not found."
    echo "Please ensure the raw traces (Azure, Alibaba, Google) are located in $DATASET_DIR"
fi

echo "Initiating Profiler..."
# Set PYTHONPATH so the profiler can resolve its modules correctly
export PYTHONPATH=$(pwd)/scripts:$PYTHONPATH

# Run the trace profiler pipeline
# This executes: Discovery -> Schema Inference -> Quality Check -> Statistics -> Semantic Mapping -> Manifest Gen
python3 scripts/trace_profiler/main.py

echo "======================================"
echo " Trace Preparation Complete."
echo " Canonical outputs and execution manifests are located in scripts/trace_profiler/output_reports/"
echo " The canonical CSV can now be ingested by embi_sim."
echo "======================================"
