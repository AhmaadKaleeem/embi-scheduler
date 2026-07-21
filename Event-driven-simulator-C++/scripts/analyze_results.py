
import os
import json
import numpy as np
from collections import defaultdict

RAW_DIR = "results/raw"
output = open("aggregated_stats.txt", "w")

def process_results():
    if not os.path.exists(RAW_DIR):
        print("No raw results found.")
        return
        
    rqs = [d for d in os.listdir(RAW_DIR) if os.path.isdir(os.path.join(RAW_DIR, d))]
    
    for rq in sorted(rqs):
        output.write(f"\n=====================================\nRQ: {rq}\n=====================================\n")
        rq_path = os.path.join(RAW_DIR, rq)
        workloads = [d for d in os.listdir(rq_path) if os.path.isdir(os.path.join(rq_path, d))]
        
        for wl in sorted(workloads):
            output.write(f"\n  Workload: {wl}\n")
            wl_path = os.path.join(rq_path, wl)
            schedulers = [d for d in os.listdir(wl_path) if os.path.isdir(os.path.join(wl_path, d))]
            
            for sched in sorted(schedulers):
                sched_path = os.path.join(wl_path, sched)
                seeds = [d for d in os.listdir(sched_path) if d.startswith("seed_")]
                
                throughputs = []
                p99s = []
                jains = []
                drifts = []
                utils = []
                avg_waits = []
                entropies = []
                cswitches = []
                hybrid_embi_rates = []
                hybrid_mw_rates = []
                
                for seed in seeds:
                    summary_file = os.path.join(sched_path, seed, "summary.json")
                    if os.path.exists(summary_file):
                        try:
                            with open(summary_file, "r") as f:
                                data = json.load(f)
                                throughputs.append(data.get("online", {}).get("throughput", 0))
                                p99s.append(data.get("latency", {}).get("p99_waiting_time", 0))
                                jains.append(data.get("fairness", {}).get("jain_index", 0))
                                drifts.append(data.get("stability", {}).get("avg_lyapunov_v", 0))
                                utils.append(data.get("online", {}).get("utilization", 0))
                                avg_waits.append(data.get("latency", {}).get("avg_waiting_time", 0))
                                entropies.append(data.get("scheduler_diag", {}).get("avg_entropy", 0))
                                cswitches.append(data.get("scheduler_diag", {}).get("context_switch_rate", 0))
                                hybrid_embi_rates.append(data.get("scheduler_diag", {}).get("hybrid_embi_ratio", 0))
                                hybrid_mw_rates.append(data.get("scheduler_diag", {}).get("hybrid_mw_ratio", 0))
                        except Exception as e:
                            pass
                
                if throughputs:
                    output.write(f"    Scheduler: {sched} (Seeds: {len(seeds)})\n")
                    output.write(f"      Throughput : {np.mean(throughputs):.4f} +/- {np.std(throughputs):.4f}\n")
                    output.write(f"      P99 Latency: {np.mean(p99s):.4f} +/- {np.std(p99s):.4f} (Min: {np.min(p99s):.4f}, Max: {np.max(p99s):.4f})\n")
                    output.write(f"      Avg Wait   : {np.mean(avg_waits):.4f}\n")
                    output.write(f"      Jain Index : {np.mean(jains):.4f}\n")
                    output.write(f"      Drift (V)  : {np.mean(drifts):.2e}\n")
                    output.write(f"      CPU Util   : {np.mean(utils):.4f}\n")
                    if np.mean(hybrid_embi_rates) > 0 or np.mean(hybrid_mw_rates) > 0:
                        output.write(f"      Hybrid Mode: EMBI={np.mean(hybrid_embi_rates):.4f}, MW={np.mean(hybrid_mw_rates):.4f}\n")

if __name__ == "__main__":
    process_results()
    output.close()

