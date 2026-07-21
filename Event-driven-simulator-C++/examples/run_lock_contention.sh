#!/usr/bin/env bash
# =============================================================================
# run_lock_contention.sh
#
# Runs the full sweep for the lock contention workload to demonstrate EMBI's
# advantage over MaxWeight in heterogeneous (asymmetric) settings.
#
# Usage:
#   chmod +x examples/run_lock_contention.sh
#   ./examples/run_lock_contention.sh
# =============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
BIN="${REPO_ROOT}/build_release/bin/embi_sim"
RESULTS="${REPO_ROOT}/results"
FIGS="${REPO_ROOT}/figs"
PYTHON="python3"

if [[ ! -x "$BIN" ]]; then
    echo "[ERROR] embi_sim not found at: $BIN"
    echo "        Build first:  cmake --build build_release --config Release"
    exit 1
fi

echo "[$(date '+%H:%M:%S')] Starting Lock Contention Sweep..."
echo "[$(date '+%H:%M:%S')] This will run EMBI, MaxWeight, RR, and FCFS with asymmetric arrival rates."

# Run the sweep using the JSON configuration
"$BIN" --experiment "${SCRIPT_DIR}/lock_contention_sweep.json" 2>&1 | tee "${RESULTS}/lock_contention.log"

echo "[$(date '+%H:%M:%S')] Sweep complete. Generating plots..."

SWEEP_JSON="${RESULTS}/lock_contention_sweep/summary.json"

if [[ -f "$SWEEP_JSON" ]]; then
    # Plot scheduler comparison (bar charts)
    "$PYTHON" "${REPO_ROOT}/scripts/compare_schedulers.py" \
        --input  "$SWEEP_JSON" \
        --output "${FIGS}/lock_contention_sweep"

    # Plot EMBI Pareto curve for lambda spreads
    "$PYTHON" "${REPO_ROOT}/scripts/plot_pareto_curve.py" \
        --input  "$SWEEP_JSON" \
        --output "${FIGS}/lock_contention_sweep"

    # Plot Fairness
    "$PYTHON" "${REPO_ROOT}/scripts/plot_fairness.py" \
        --input  "$SWEEP_JSON" \
        --output "${FIGS}/lock_contention_sweep/fairness"

    # Plot Latency CDFs and distributions
    "$PYTHON" "${REPO_ROOT}/scripts/plot_latency.py" \
        --input  "$SWEEP_JSON" \
        --output "${FIGS}/lock_contention_sweep/latency"

    echo "[$(date '+%H:%M:%S')] Plots generated in ${FIGS}/lock_contention_sweep/"
else
    echo "[ERROR] summary.json not found. Did the sweep fail?"
fi

echo "[$(date '+%H:%M:%S')] All done!"
