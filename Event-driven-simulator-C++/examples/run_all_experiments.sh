#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" &> /dev/null && pwd)"
REPO_ROOT="$(dirname "$SCRIPT_DIR")"
BIN="${REPO_ROOT}/build_release/bin/embi_sim"
FIGS="${REPO_ROOT}/figs"
PYTHON="python3"

if [ ! -f "$BIN" ]; then
    echo "[ERROR] embi_sim not found at: $BIN"
    echo "        You must build the project first!"
    echo "        Run: cmake -S . -B build_release -DCMAKE_BUILD_TYPE=Release"
    echo "        Run: cmake --build build_release -j"
    exit 1
fi

echo "[INFO] Running Exp 1: Symmetric Validation..."
"$BIN" --experiment "${SCRIPT_DIR}/exp1_symmetric.json"

echo "[INFO] Running Exp 2: Asymmetric Sweep..."
"$BIN" --experiment "${SCRIPT_DIR}/exp2_asymmetric.json"

echo "[INFO] Running Exp 3: High Contention..."
"$BIN" --experiment "${SCRIPT_DIR}/exp3_high_contention.json"

echo "[INFO] Running Exp 4: Long Hold Times..."
"$BIN" --experiment "${SCRIPT_DIR}/exp4_long_hold.json"

echo "[INFO] Running Exp 5: Scalability..."
"$BIN" --experiment "${SCRIPT_DIR}/exp5_scalability.json"

echo "[INFO] Generating PDF Figures..."
"$PYTHON" "${REPO_ROOT}/scripts/compare_schedulers.py" --input "${REPO_ROOT}/results/exp2_asymmetric/summary.json" --output "$FIGS" --format pdf
"$PYTHON" "${REPO_ROOT}/scripts/plot_pareto_curve.py" --input "${REPO_ROOT}/results/exp2_asymmetric/summary.json" --output "$FIGS"
"$PYTHON" "${REPO_ROOT}/scripts/plot_fairness.py" --input "${REPO_ROOT}/results/exp2_asymmetric/summary.json" --output "$FIGS" --format pdf
"$PYTHON" "${REPO_ROOT}/scripts/plot_latency.py" --input "${REPO_ROOT}/results/exp3_high_contention/summary.json" --output "$FIGS" --format pdf

echo "[SUCCESS] All experiments and plotting complete! Check the figs/ folder."
