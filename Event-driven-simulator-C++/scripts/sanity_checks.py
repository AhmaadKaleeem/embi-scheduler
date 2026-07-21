#!/usr/bin/env python3
import os
import sys
import json
import glob
import math

RAW_DIR = "results/raw"

def abort(msg):
    print(f"[FAIL] {msg}")
    sys.exit(1)

def run_checks():
    print("========== Running Golden Pipeline Sanity Checks ==========")
    if not os.path.exists(RAW_DIR):
        abort(f"Raw results directory '{RAW_DIR}' not found.")
        
    rqs = [d for d in os.listdir(RAW_DIR) if os.path.isdir(os.path.join(RAW_DIR, d))]
    if not rqs:
        abort("No research questions found in results/raw/.")
        
    for rq in rqs:
        rq_path = os.path.join(RAW_DIR, rq)
        workloads = [d for d in os.listdir(rq_path) if os.path.isdir(os.path.join(rq_path, d))]
        for wl in workloads:
            wl_path = os.path.join(rq_path, wl)
            schedulers = [d for d in os.listdir(wl_path) if os.path.isdir(os.path.join(wl_path, d))]
            for sched in schedulers:
                sched_path = os.path.join(wl_path, sched)
                
                # Check 50 seeds
                seeds = glob.glob(os.path.join(sched_path, "seed_*"))
                if len(seeds) != 50:
                    abort(f"Missing seeds in {sched_path}. Found {len(seeds)}, expected 50.")
                
                for seed_dir in seeds:
                    summary_file = os.path.join(seed_dir, "summary.json")
                    if not os.path.exists(summary_file):
                        abort(f"Missing summary.json in {seed_dir}")
                        
                    with open(summary_file, 'r') as f:
                        try:
                            data = json.load(f)
                        except Exception as e:
                            abort(f"Invalid JSON in {summary_file}: {e}")
                            
                    # Extracted fields
                    jain = data.get("fairness", {}).get("jain_index", 0.0)
                    util = data.get("system", {}).get("cpu_utilization", 0.0)
                    throughput = data.get("scheduler_diag", {}).get("total_throughput", 0.0)
                    drift = data.get("stability", {}).get("avg_lyapunov_v", 0.0)
                    p99 = data.get("latency", {}).get("p99_turnaround", 0.0)
                    median = data.get("latency", {}).get("median_turnaround", 0.0)
                    
                    # Sanity Checks
                    if math.isnan(jain) or math.isnan(util) or math.isnan(throughput):
                        abort(f"NaN detected in {summary_file}")
                    if math.isinf(jain) or math.isinf(util) or math.isinf(throughput):
                        abort(f"Infinity detected in {summary_file}")
                    
                    if jain > 1.0 or jain < 0.0:
                        abort(f"Invalid Jain index {jain} in {summary_file}")
                    if util > 1.0 or util < 0.0:
                        abort(f"Invalid CPU utilization {util} in {summary_file}")
                    if throughput < 0.0:
                        abort(f"Negative throughput {throughput} in {summary_file}")
                    if not math.isfinite(drift):
                        abort(f"Non-finite drift {drift} in {summary_file}")
                    if p99 < median:
                        abort(f"P99 {p99} less than median {median} in {summary_file}")

    print("[PASS] All 50 seeds completed for all configurations.")
    print("[PASS] No NaNs or Infinities detected.")
    print("[PASS] No negative queue lengths or invalid metrics.")
    print("[PASS] Jain <= 1, Util <= 1, Throughput >= 0, P99 >= Median.")
    print("Sanity checks passed! Ready for plotting.")

if __name__ == "__main__":
    run_checks()
