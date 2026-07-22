import os
import json
import numpy as np

def main():
    raw_dir = "results/raw"
    metrics_to_collect = {
        'Mean': 'avg_waiting_time',
        'Median': 'p50_waiting_time',
        'P95': 'p95_waiting_time',
        'P99': 'p99_waiting_time',
        'Throughput': 'throughput',
        'ContextSwitches': 'context_switch_rate',
        'Fairness': 'jain_index'
    }
    
    schedulers = ['fcfs', 'rr', 'maxweight', 'cmu', 'embi', 'hybrid_embi', 'embi_oracle']
    workloads = [('uniform', '0.9'), ('poisson', '0.9'), ('bursty', '0.9'), ('heavy_tail', '0.9'), ('human_trace', '1.0')]
    
    # Nested dict: data[metric][workload][scheduler] = list of values
    data = {m: {wl: {s: [] for s in schedulers} for wl, _ in workloads} for m in metrics_to_collect}
    
    for root, _, files in os.walk(raw_dir):
        if "summary.json" in files:
            parts = root.replace("\\", "/").split('/')
            if len(parts) >= 6:
                phase = parts[-5]
                wl = parts[-4]
                rate = parts[-3].replace('rate_', '')
                sched = parts[-2]
                
                # Check if it matches our target configs
                is_target = False
                for target_wl, target_rate in workloads:
                    if wl == target_wl and rate == target_rate:
                        is_target = True
                        break
                
                if not is_target or sched not in schedulers:
                    continue
                    
                path = os.path.join(root, "summary.json")
                try:
                    with open(path, 'r', encoding='utf-8') as f:
                        d = json.load(f)
                        if isinstance(d, list): d = d[0]
                        
                        lat = d.get('latency', {})
                        online = d.get('online', {})
                        fair = d.get('fairness', {})
                        diag = d.get('scheduler_diag', {})
                        
                        val_map = {
                            'Mean': lat.get('avg_waiting_time', 0),
                            'Median': lat.get('p50_waiting_time', 0),
                            'P95': lat.get('p95_waiting_time', 0),
                            'P99': lat.get('p99_waiting_time', 0),
                            'Throughput': online.get('throughput', 0),
                            'ContextSwitches': diag.get('context_switch_rate', 0),
                            'Fairness': fair.get('jain_index', 0)
                        }
                        
                        for m in metrics_to_collect:
                            data[m][wl][sched].append(val_map[m])
                            
                except Exception as e:
                    pass

    # Generate Markdown Tables
    out_lines = ["# Final Diagnostic Tables (Rate = 0.9 for synthetics, 1.0 for Trace)\n"]
    
    sched_display = ['FCFS', 'RR', 'MW', 'CMU', 'EMBI', 'Hybrid', 'Oracle']
    
    for metric in metrics_to_collect:
        out_lines.append(f"## {metric}")
        header = f"| Workload | " + " | ".join(sched_display) + " | Winner |"
        out_lines.append(header)
        out_lines.append("|---" * (len(sched_display) + 2) + "|")
        
        for wl, _ in workloads:
            row = [wl]
            vals = []
            valid_scheds = []
            
            for s in schedulers:
                lst = data[metric][wl][s]
                if len(lst) > 0:
                    v = np.mean(lst)
                    vals.append(v)
                    valid_scheds.append((s, v))
                    if metric in ['Throughput', 'Fairness']:
                        row.append(f"{v:.4f}")
                    else:
                        row.append(f"{v:.2f}")
                else:
                    row.append("-")
            
            # Determine winner
            if valid_scheds:
                if metric in ['Throughput', 'Fairness']:
                    best = max(valid_scheds, key=lambda x: x[1])[0]
                else:
                    best = min(valid_scheds, key=lambda x: x[1])[0]
                # Format winner name
                if best == 'hybrid_embi': best_str = 'HYBRID'
                elif best == 'embi_oracle': best_str = 'ORACLE'
                else: best_str = best.upper()
                row.append(f"**{best_str}**")
            else:
                row.append("-")
                
            out_lines.append("| " + " | ".join(row) + " |")
            
        out_lines.append("\n")

    with open("diagnostic_tables.md", "w") as f:
        f.write("\n".join(out_lines))
        
    print("Diagnostics written to diagnostic_tables.md")

if __name__ == '__main__':
    main()
