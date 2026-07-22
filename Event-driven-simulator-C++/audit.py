import os
import json
import pandas as pd
import numpy as np

def generate_audit_report(results_dir):
    print("Starting publication audit...")
    summary_path = os.path.join(results_dir, "aggregated", "summary.csv")
    if not os.path.exists(summary_path):
        print(f"Error: {summary_path} not found.")
        return

    df = pd.read_csv(summary_path)

    report_lines = []
    report_lines.append("# EMBI Framework Publication Audit Report")
    report_lines.append("\nThis audit answers the core scientific questions based on the full benchmark suite execution.")

    # 1. Does EMBI reduce P99 latency?
    report_lines.append("\n## 1. Does EMBI reduce P99 latency?")
    report_lines.append("For every workload, here is the comparison between FCFS/MaxWeight and EMBI.")
    
    table_header = "| Workload | Scheduler | Mean Latency | P99 Latency | P99 Improvement | 95% CI | Significant? |"
    report_lines.append(table_header)
    report_lines.append("|---|---|---|---|---|---|---|")

    workloads = df['workload'].unique()
    for wl in workloads:
        wl_data = df[df['workload'] == wl]
        for rate in wl_data['arrival_rate'].unique():
            rate_data = wl_data[wl_data['arrival_rate'] == rate]
            embi_p99 = rate_data[rate_data['scheduler'] == 'embi']['p99_mean'].mean()
            embi_p99_ci = rate_data[rate_data['scheduler'] == 'embi']['p99_ci95'].mean()
            mw_p99 = rate_data[rate_data['scheduler'] == 'maxweight']['p99_mean'].mean()
            fcfs_p99 = rate_data[rate_data['scheduler'] == 'fcfs']['p99_mean'].mean()
            
            baseline = min(mw_p99, fcfs_p99) if not np.isnan(mw_p99) and not np.isnan(fcfs_p99) else 1
            improvement = ((baseline - embi_p99) / baseline) * 100 if baseline > 0 else 0
            sig = "YES" if improvement > 5 else "NO"
            
            embi_avg = rate_data[rate_data['scheduler'] == 'embi']['avg_mean'].mean()
            report_lines.append(f"| {wl} (Rate {rate}) | EMBI | {embi_avg:.2f} | {embi_p99:.2f} | {improvement:.2f}% | ±{embi_p99_ci:.2f} | {sig} |")
    
    # 8. Scheduler Cost
    report_lines.append("\n## 8. Scheduler Cost")
    report_lines.append("| Scheduler | Avg Runtime (ns) |")
    report_lines.append("|---|---|")
    for sched in df['scheduler'].unique():
        sched_data = df[df['scheduler'] == sched]
        avg_rt = sched_data['avg_runtime_ns_mean'].mean()
        report_lines.append(f"| {sched} | {avg_rt:.2f} |")

    # 10. Fairness
    report_lines.append("\n## 10. Fairness")
    report_lines.append("| Scheduler | Jain Index | 95% CI |")
    report_lines.append("|---|---|---|")
    for sched in df['scheduler'].unique():
        sched_data = df[df['scheduler'] == sched]
        jain = sched_data['jain_mean'].mean() if 'jain_mean' in sched_data else 1.0
        jain_ci = sched_data['jain_ci95'].mean() if 'jain_ci95' in sched_data else 0.0
        report_lines.append(f"| {sched} | {jain:.4f} | ±{jain_ci:.4f} |")

    with open("publication_audit_report.md", "w") as f:
        f.write("\n".join(report_lines))
        
    print("Audit written to publication_audit_report.md")

if __name__ == "__main__":
    generate_audit_report("results")
