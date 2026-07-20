#!/usr/bin/env python3
"""
plot_fairness.py - Jain Fairness Index comparison across schedulers and workloads.

Usage:
    python scripts/plot_fairness.py --input results/summary.json --output figs/
"""

import argparse
import json
import os
import sys
import numpy as np
import matplotlib.pyplot as plt

plt.rcParams.update({
    "font.family":       "DejaVu Sans",
    "font.size":         11,
    "axes.spines.top":   False,
    "axes.spines.right": False,
    "figure.dpi":        150,
})

_PALETTE = ["#4C72B0", "#DD8452", "#55A868", "#C44E52",
            "#8172B2", "#937860", "#DA8BC3", "#8C8C8C"]

def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Plot fairness metrics")
    p.add_argument("--input",    required=True, help="Path to experiment summary.json")
    p.add_argument("--output",   default="figs", help="Output directory")
    p.add_argument("--format",   default="png", choices=["png", "pdf", "svg"])
    p.add_argument("--title",    default=None)
    return p.parse_args()

def load_data(path: str) -> list[dict]:
    with open(path) as f:
        data = json.load(f)
    if not isinstance(data, list):
        raise ValueError("plot_fairness expects an experiment summary.json array")
    return data

def plot_grouped_bars(data: list[dict], args, out_dir: str) -> None:
    schedulers = sorted({r.get("scheduler", "?") for r in data})
    workloads  = sorted({r.get("workload",  "?") for r in data})

    jain = {}
    for sched in schedulers:
        for wload in workloads:
            vals = [r["jain"] for r in data
                    if r.get("scheduler") == sched and r.get("workload") == wload
                    and "jain" in r]
            jain[(sched, wload)] = np.mean(vals) if vals else 0.0

    x = np.arange(len(workloads))
    width = 0.8 / max(len(schedulers), 1)
    offsets = np.linspace(-(len(schedulers) - 1) * width / 2,
                           (len(schedulers) - 1) * width / 2,
                           len(schedulers))

    fig, ax = plt.subplots(figsize=(max(8, len(workloads) * 2), 5))
    for i, sched in enumerate(schedulers):
        vals = [jain.get((sched, wl), 0.0) for wl in workloads]
        ax.bar(x + offsets[i], vals, width * 0.9,
               label=sched, color=_PALETTE[i % len(_PALETTE)])

    ax.set_xticks(x)
    ax.set_xticklabels(workloads)
    ax.set_ylabel("Jain Fairness Index")
    ax.set_ylim(0, 1.05)
    ax.axhline(1.0, color="gray", linewidth=0.7, linestyle="--")
    ax.set_title(args.title or "Jain Fairness Index: Scheduler x Workload", pad=12)
    ax.legend(fontsize=9)
    ax.grid(axis="y", linewidth=0.4, alpha=0.5)
    plt.tight_layout()

    out = os.path.join(out_dir, f"fairness_bars.{args.format}")
    fig.savefig(out, bbox_inches="tight")
    print(f"[plot_fairness] Saved -> {out}")
    plt.close(fig)

def plot_jain_vs_rate(data: list[dict], args, out_dir: str) -> None:
    schedulers = sorted({r.get("scheduler", "?") for r in data})
    all_rates  = sorted({r.get("arrival_rate", 0.0) for r in data})

    if len(all_rates) < 2:
        return

    fig, ax = plt.subplots(figsize=(10, 5))
    for i, sched in enumerate(schedulers):
        rates, jains, stds = [], [], []
        for rate in all_rates:
            vals = [r["jain"] for r in data
                    if r.get("scheduler") == sched and
                    abs(r.get("arrival_rate", -1) - rate) < 1e-9 and
                    "jain" in r]
            if vals:
                rates.append(rate)
                jains.append(np.mean(vals))
                stds.append(np.std(vals))

        if not rates:
            continue
        color = _PALETTE[i % len(_PALETTE)]
        ax.plot(rates, jains, "o-", color=color, linewidth=1.5, label=sched)

    ax.axhline(1.0, color="gray", linewidth=0.7, linestyle="--")
    ax.set_xlabel("Arrival Rate")
    ax.set_ylabel("Jain Fairness Index")
    ax.set_title("Jain Fairness vs. Arrival Rate", pad=12)
    ax.legend(fontsize=9)
    ax.grid(linewidth=0.4, alpha=0.5)
    plt.tight_layout()

    out = os.path.join(out_dir, f"fairness_vs_rate.{args.format}")
    fig.savefig(out, bbox_inches="tight")
    print(f"[plot_fairness] Saved -> {out}")
    plt.close(fig)

def plot_fairness(args: argparse.Namespace) -> None:
    data = load_data(args.input)
    os.makedirs(args.output, exist_ok=True)
    plot_grouped_bars(data, args, args.output)
    plot_jain_vs_rate(data, args, args.output)
    print(f"[plot_fairness] All figures saved to {args.output}/")

if __name__ == "__main__":
    plot_fairness(parse_args())
