#!/usr/bin/env python3
"""
compare_schedulers.py - Generates comparison plots (radar charts, table) from summary.json.

Usage:
    python scripts/compare_schedulers.py --input results/summary.json --output figs/
"""

import argparse
import json
import os
import sys
import numpy as np
import matplotlib.pyplot as plt
from math import pi

def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser()
    p.add_argument("--input", required=True)
    p.add_argument("--output", default="figs")
    p.add_argument("--metrics", nargs="+", default=["jain", "avg_wait", "p99", "throughput"])
    p.add_argument("--format", default="png")
    return p.parse_args()

def load_data(path: str) -> list[dict]:
    with open(path) as f:
        data = json.load(f)
    return data if isinstance(data, list) else [data]

def aggregate(data: list[dict]) -> dict:
    agg = {}
    for r in data:
        s = r.get("scheduler", "?")
        if s not in agg:
            agg[s] = {"jain": [], "avg_wait": [], "p99": [], "throughput": [], "utilization": [], "starvation": []}
        
        agg[s]["jain"].append(r.get("jain", 0.0))
        lat = r.get("latency", {})
        agg[s]["avg_wait"].append(lat.get("avg_waiting_time", 0.0))
        agg[s]["p99"].append(lat.get("p99_waiting_time", lat.get("p99", 0.0)))
        agg[s]["throughput"].append(r.get("total_throughput", 0.0))
        agg[s]["utilization"].append(r.get("total_utilization", 0.0))
        agg[s]["starvation"].append(r.get("max_starvation", 0.0))
        
    res = {}
    for k, v in agg.items():
        res[k] = {m: np.mean(arr) if arr else 0.0 for m, arr in v.items()}
    return res

def plot_radar(agg: dict, metrics: list[str], args, out_dir: str):
    scheds = list(agg.keys())
    if not scheds or not metrics:
        return
        
    N = len(metrics)
    angles = [n / float(N) * 2 * pi for n in range(N)]
    angles += angles[:1]
    
    fig, ax = plt.subplots(figsize=(6, 6), subplot_kw=dict(polar=True))
    ax.set_theta_offset(pi / 2)
    ax.set_theta_direction(-1)
    plt.xticks(angles[:-1], metrics, size=10)
    ax.set_rlabel_position(0)
    
    # Normalize metrics to [0, 1] for radar chart
    max_vals = {m: max((agg[s][m] for s in scheds), default=1e-9) for m in metrics}
    for m in max_vals:
        if max_vals[m] == 0: max_vals[m] = 1.0

    colors = ["#4C72B0", "#DD8452", "#55A868", "#C44E52", "#8172B2"]
    
    for i, s in enumerate(scheds):
        vals = [agg[s][m] / max_vals[m] for m in metrics]
        # Invert "bad" metrics so larger area = better
        for j, m in enumerate(metrics):
            if m in ["avg_wait", "p99", "starvation"]:
                vals[j] = 1.0 - vals[j]
                
        vals += vals[:1]
        c = colors[i % len(colors)]
        ax.plot(angles, vals, linewidth=2, linestyle='solid', label=s, color=c)
        ax.fill(angles, vals, color=c, alpha=0.25)
        
    plt.legend(loc='upper right', bbox_to_anchor=(1.2, 1.1))
    plt.title("Normalized Scheduler Performance\n(Larger area = Better)", y=1.08)
    
    out = os.path.join(out_dir, f"radar_comparison.{args.format}")
    fig.savefig(out, bbox_inches="tight")
    print(f"[compare_schedulers] Saved -> {out}")
    plt.close(fig)

def print_table(agg: dict):
    print("\n" + "="*109)
    print(f"{'Scheduler':<14}{'Jain Fairness':<14}{'Avg Wait (ticks)':<18}{'P99 Latency':<14}{'Throughput':<22}{'CPU Util':<14}{'Max Starv':<14}")
    print("="*109)
    for s, m in agg.items():
        print(f"{s:<14}{m['jain']:<14.4f}{m['avg_wait']:<18.4f}{m['p99']:<14.4f}{m['throughput']:<22.4f}{m['utilization']:<14.4f}{m['starvation']:<14.4f}")
    print("="*109 + "\n")

def main(args):
    data = load_data(args.input)
    agg = aggregate(data)
    os.makedirs(args.output, exist_ok=True)
    plot_radar(agg, args.metrics, args, args.output)
    print_table(agg)

if __name__ == "__main__":
    main(parse_args())
