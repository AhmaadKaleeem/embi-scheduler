import os
import json
import csv
import numpy as np
import scipy.stats as stats

def compute_ci95(data):
    if len(data) < 2:
        return 0.0
    a = 1.0 * np.array(data)
    m, se = np.mean(a), stats.sem(a)
    return se * stats.t.ppf((1 + 0.95) / 2., len(a)-1)

def compute_cohens_d(group1, group2):
    n1, n2 = len(group1), len(group2)
    if n1 < 2 or n2 < 2: return 0.0
    var1, var2 = np.var(group1, ddof=1), np.var(group2, ddof=1)
    pooled_var = ((n1 - 1) * var1 + (n2 - 1) * var2) / (n1 + n2 - 2)
    return (np.mean(group1) - np.mean(group2)) / np.sqrt(pooled_var)

def main():
    print("Aggregating results...")
    raw_dir = "results/raw"
    out_dir = "results/aggregated"
    os.makedirs(out_dir, exist_ok=True)
    os.makedirs("results/paper/figures", exist_ok=True)
    os.makedirs("results/paper/tables", exist_ok=True)

    summary_file = os.path.join(out_dir, "summary.csv")
    
    # We will collect data grouped by (experiment, workload, scheduler)
    # inside which we have lists of metrics across seeds.
    grouped_data = {}
    
    for root, dirs, files in os.walk(raw_dir):
        if "summary.json" in files:
            json_path = os.path.join(root, "summary.json")
            try:
                with open(json_path, 'r', encoding='utf-8') as f:
                    data = json.load(f)
                    if not data: continue
                    
                    # Assume results/raw/<phase>/<workload>/rate_<rate>/<scheduler>/seed_<seed>
                    parts = root.replace("\\", "/").split('/')
                    if len(parts) >= 6:
                        experiment = parts[-5]
                        workload = parts[-4]
                        # Rate is captured in config parsing anyway, but we get scheduler from parts
                        scheduler = parts[-2]
                        seed = parts[-1].replace('seed_', '')
                    else:
                        continue
                        
                    # Handle both list and object formats
                    d = data[0] if isinstance(data, list) else data
                    
                    key = (experiment, workload, scheduler)
                    if key not in grouped_data:
                        grouped_data[key] = {
                            "M": d.get("config", {}).get("M", 0),
                            "arrival_rate": d.get("config", {}).get("arrival_rate", 0),
                            "avg_waiting_time": [],
                            "p99_waiting_time": [],
                            "jain_index": [],
                            "queue_p99": [],
                            "avg_runtime_ns": [],
                            "mw_divergence_ratio": [],
                            "lambda_mse": [],
                            "context_switches": [],
                        }
                    
                    gd = grouped_data[key]
                    
                    # Extract latency metrics
                    lat = d.get("latency", {})
                    gd["avg_waiting_time"].append(lat.get("avg_waiting_time", d.get("avg", 0)))
                    gd["p99_waiting_time"].append(lat.get("p99_waiting_time", d.get("p99", 0)))
                    
                    # Extract fairness metrics
                    fair = d.get("fairness", {})
                    gd["jain_index"].append(fair.get("jain_index", d.get("jain", 0)))
                    
                    # Extract queue metrics
                    q = d.get("queue", {})
                    gd["queue_p99"].append(q.get("p99", d.get("queue_p99", 0)))
                    
                    # Extract scheduler diagnostic metrics
                    diag = d.get("scheduler_diag", {})
                    gd["avg_runtime_ns"].append(diag.get("avg_runtime_ns", d.get("avg_scheduler_runtime_ns", 0)))
                    gd["mw_divergence_ratio"].append(diag.get("hybrid_mw_ratio", d.get("mw_divergence_ratio", 0)))
                    
                    # Other top-level metrics
                    gd["lambda_mse"].append(d.get("lambda_mse", 0))
                    gd["context_switches"].append(d.get("context_switches", 0))
                    
            except Exception as e:
                print(f"Failed to read {json_path}: {e}")

    if not grouped_data:
        print("No summary.json files found.")
        return

    # Now compute aggregated stats
    with open(summary_file, 'w', newline='', encoding='utf-8') as f:
        writer = csv.writer(f)
        writer.writerow([
            "experiment", "workload", "scheduler", "M", "arrival_rate", "N_seeds",
            "avg_mean", "avg_ci95", "p99_mean", "p99_ci95", "jain_mean", "jain_ci95", "queue_p99_mean", 
            "mw_divergence_mean", "lambda_mse_mean", "avg_runtime_ns_mean", "context_switches_mean"
        ])
        
        for (exp, wl, sched), metrics in grouped_data.items():
            writer.writerow([
                exp, wl, sched, metrics["M"], metrics["arrival_rate"], len(metrics["p99_waiting_time"]),
                np.mean(metrics["avg_waiting_time"]), compute_ci95(metrics["avg_waiting_time"]),
                np.mean(metrics["p99_waiting_time"]), compute_ci95(metrics["p99_waiting_time"]),
                np.mean(metrics["jain_index"]), compute_ci95(metrics["jain_index"]),
                np.mean(metrics["queue_p99"]),
                np.mean(metrics["mw_divergence_ratio"]),
                np.mean(metrics["lambda_mse"]),
                np.mean(metrics["avg_runtime_ns"]),
                np.mean(metrics["context_switches"])
            ])
            
    print(f"Aggregation complete. Saved to {summary_file}")

if __name__ == "__main__":
    main()
