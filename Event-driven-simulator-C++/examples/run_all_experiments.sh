#!/usr/bin/env bash
# =============================================================================
# run_all_experiments.sh
#
# Orchestrates the full EMBI research experiment suite:
#
#   Phase 1 — Quick smoke test          (6 runs, ~10 s)
#   Phase 2 — Scheduler comparison      (24 runs, ~5 min)
#   Phase 3 — Full research sweep       (1080 runs, ~2-4 h)
#   Phase 4 — Plotting
#
# Usage:
#   chmod +x examples/run_all_experiments.sh
#   ./examples/run_all_experiments.sh
#   ./examples/run_all_experiments.sh --phase 1        # smoke test only
#   ./examples/run_all_experiments.sh --phase 2        # scheduler compare
#   ./examples/run_all_experiments.sh --phase 3        # full sweep
#   ./examples/run_all_experiments.sh --phase 4        # plots only
#   ./examples/run_all_experiments.sh --bin path/to/embi_sim
#
# =============================================================================
set -euo pipefail

# ─── Defaults ─────────────────────────────────────────────────────────────────
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
BIN="${REPO_ROOT}/build_release/bin/embi_sim"
PYTHON="python3"
RESULTS="${REPO_ROOT}/results"
FIGS="${REPO_ROOT}/figs"
PHASE="all"

# ─── Argument parsing ─────────────────────────────────────────────────────────
while [[ $# -gt 0 ]]; do
    case "$1" in
        --bin)    BIN="$2";   shift 2 ;;
        --phase)  PHASE="$2"; shift 2 ;;
        --python) PYTHON="$2";shift 2 ;;
        --results) RESULTS="$2"; shift 2 ;;
        --figs)    FIGS="$2";    shift 2 ;;
        *) echo "Unknown flag: $1"; exit 1 ;;
    esac
done

# ─── Sanity check ─────────────────────────────────────────────────────────────
if [[ "$PHASE" != "4" ]] && [[ ! -x "$BIN" ]]; then
    echo "[ERROR] embi_sim not found at: $BIN"
    echo "        Build first:  cmake --build build_release --config Release"
    exit 1
fi

mkdir -p "$RESULTS" "$FIGS"

log() { echo "[$(date '+%H:%M:%S')] $*"; }

# ─── Phase 1: Smoke test ──────────────────────────────────────────────────────
run_phase1() {
    log "Phase 1 — Smoke test (6 runs)"
    "$BIN" --experiment "${SCRIPT_DIR}/quick_demo.json" 2>&1 | tee "${RESULTS}/phase1.log"
    log "Phase 1 complete."
}

# ─── Phase 2: Scheduler comparison (all schedulers × poisson × 4 seeds) ──────
run_phase2() {
    log "Phase 2 — Scheduler comparison (24 runs)"
    PHASE2_DIR="${RESULTS}/scheduler_compare"
    mkdir -p "$PHASE2_DIR"

    TMP_EXP="$(mktemp /tmp/embi_exp_XXXX.json)"
    cat > "$TMP_EXP" << 'EXPEOF'
{
  "schedulers":    ["embi", "embi_unclipped", "maxweight", "cmu", "rr", "fcfs"],
  "workloads":     ["poisson"],
  "seeds":         [42, 123, 456, 789],
  "arrival_rates": [0.5],
  "ticks":         500000,
  "num_processes": 16,
  "M":             10.0,
  "service_rate":  1.0,
  "alpha":         0.1,
  "beta":          0.1,
  "log_freq":      1000,
  "null_log":      false,
  "output_dir":    "results/scheduler_compare"
}
EXPEOF

    "$BIN" --experiment "$TMP_EXP" 2>&1 | tee "${RESULTS}/phase2.log"
    rm -f "$TMP_EXP"
    log "Phase 2 complete. Output in ${PHASE2_DIR}/"
}

# ─── Phase 3: Full research sweep ─────────────────────────────────────────────
run_phase3() {
    log "Phase 3 — Full research sweep (1080 runs). This may take 2-4 hours."
    "$BIN" --experiment "${SCRIPT_DIR}/full_sweep.json" 2>&1 | tee "${RESULTS}/phase3.log"
    log "Phase 3 complete."
}

# ─── Phase 4: Generate all plots ─────────────────────────────────────────────
run_phase4() {
    log "Phase 4 — Generating plots"

    # Helper: only plot if the summary.json exists
    maybe_plot() {
        local summary="$1"; shift
        if [[ -f "$summary" ]]; then
            "$PYTHON" "$@" || true
        else
            log "  [skip] $summary not found"
        fi
    }

    # Phase 2 plots
    P2_JSON="${RESULTS}/scheduler_compare/summary.json"
    maybe_plot "$P2_JSON" \
        "${REPO_ROOT}/scripts/compare_schedulers.py" \
        --input  "$P2_JSON" \
        --output "${FIGS}/scheduler_compare" \
        --workload poisson

    maybe_plot "$P2_JSON" \
        "${REPO_ROOT}/scripts/plot_fairness.py" \
        --input  "$P2_JSON" \
        --output "${FIGS}/fairness"

    maybe_plot "$P2_JSON" \
        "${REPO_ROOT}/scripts/plot_latency.py" \
        --input  "$P2_JSON" \
        --output "${FIGS}/latency"

    # Full sweep plots
    FULL_JSON="${RESULTS}/full_sweep/summary.json"
    maybe_plot "$FULL_JSON" \
        "${REPO_ROOT}/scripts/compare_schedulers.py" \
        --input  "$FULL_JSON" \
        --output "${FIGS}/full_sweep"

    # Per-run queue / drift plots for the first run found
    FIRST_CSV="$(find "${RESULTS}" -name "run.csv" | head -1 || true)"
    if [[ -n "$FIRST_CSV" ]]; then
        log "  Plotting queue/drift for: $FIRST_CSV"
        "$PYTHON" "${REPO_ROOT}/scripts/plot_queue.py" \
            --input  "$FIRST_CSV" \
            --output "${FIGS}/queue" \
            --smooth 200 || true

        "$PYTHON" "${REPO_ROOT}/scripts/plot_drift.py" \
            --input  "$FIRST_CSV" \
            --output "${FIGS}/drift" \
            --smooth 500 || true
    fi

    log "Phase 4 complete. Figures in ${FIGS}/"
}

# ─── Dispatch ─────────────────────────────────────────────────────────────────
case "$PHASE" in
    1|"smoke")   run_phase1 ;;
    2|"compare") run_phase2 ;;
    3|"full")    run_phase3 ;;
    4|"plots")   run_phase4 ;;
    "all")
        run_phase1
        run_phase2
        run_phase3
        run_phase4
        ;;
    *)
        echo "Unknown phase: $PHASE (expected 1|2|3|4|all)"
        exit 1
        ;;
esac

log "All done. Results: ${RESULTS}/"
log "           Figures: ${FIGS}/"
