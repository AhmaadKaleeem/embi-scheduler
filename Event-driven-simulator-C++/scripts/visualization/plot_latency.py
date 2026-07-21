#!/usr/bin/env python3
"""
plot_latency.py - Waiting-time CDF and histogram from summary.json.

Usage:
    python scripts/plot_latency.py --input results/summary.json --output figs/
    python scripts/plot_latency.py --input results/summary.json --schedulers embi rr

Reads the JSON summary produced by StatisticsDatabase::exportJSONSummary()
and plots a waiting-time CDF, histogram, and P50/P95/P99 bar chart.
"""

import argparse
import json
import os
import sys
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
from pathlib import Path

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
    p = argparse.ArgumentParser(description="Plot waiting-time latency")
    p.add_argument("--input",      required=True,
                   help="Path to summary.json (single run or experiment array)")
    p.add_argument("--output",     default="figs", help="Output directory")
    p.add_argument("--schedulers", nargs="+", default=None,
                   help="Filter by scheduler names")
    p.add_argument("--workload",   default=None, help="Filter by workload name")
    p.add_argument("--format",     default="png", choices=["png", "pdf", "svg"])
    p.add_argument("--title",      default=None)
    return p.parse_args()


def load_summaries(path: str) -> list[dict]:
    with open(path) as f:
        data = json.load(f)
    if isinstance(data, list):
        return data
    return [data]


def plot_percentile_bars(summaries: list[dict], args: argparse.Namespace, out_dir: str) -> None:
    labels, p50s, p95s, p99s = [], [], [], []

    for s in summaries:
        sched = s.get("scheduler", s.get("config", {}).get("scheduler", "?"))
        wload = s.get("workload",  s.get("config", {}).get("workload",  "?"))

        lat = s.get("latency", s)
        labels.append(f"{sched}\n{wload}")
        p50s.append(lat.get("p50_waiting_time",  lat.get("p50", 0.0)))
        p95s.append(lat.get("p95_waiting_time",  lat.get("p95", 0.0)))
        p99s.append(lat.get("p99_waiting_time",  lat.get("p99", 0.0)))

    x = np.arange(len(labels))
    width = 0.25

    fig, ax = plt.subplots(figsize=(max(8, len(labels) * 1.6), 5))
    ax.bar(x - width, p50s, width, label="P50", color=_PALETTE[0])
    ax.bar(x,         p95s, width, label="P95", color=_PALETTE[1])
    ax.bar(x + width, p99s, width, label="P99", color=_PALETTE[3])

    ax.set_xticks(x)
    ax.set_xticklabels(labels, fontsize=9)
    ax.set_ylabel("Waiting Time (ticks)")
    ax.set_title(args.title or "Latency Percentiles by Scheduler", pad=12)
    ax.legend()
    ax.grid(axis="y", linewidth=0.4, alpha=0.5)

    plt.tight_layout()
    out = os.path.join(out_dir, f"latency_percentiles.{args.format}")
    fig.savefig(out, bbox_inches="tight")
    print(f"[plot_latency] Saved -> {out}")
    plt.close(fig)


def plot_latency(args: argparse.Namespace) -> None:
    summaries = load_summaries(args.input)

    if args.schedulers:
        summaries = [s for s in summaries
                     if s.get("scheduler", s.get("config", {}).get("scheduler")) in args.schedulers]
    if args.workload:
        summaries = [s for s in summaries
                     if s.get("workload",  s.get("config", {}).get("workload"))  == args.workload]

    if not summaries:
        print("[plot_latency] No matching runs found", file=sys.stderr)
        return

    os.makedirs(args.output, exist_ok=True)
    plot_percentile_bars(summaries, args, args.output)

    fig, ax = plt.subplots(figsize=(10, 5))
    for i, s in enumerate(summaries):
        lat   = s.get("latency", s)
        sched = s.get("scheduler", s.get("config", {}).get("scheduler", "?"))
        wload = s.get("workload",  s.get("config", {}).get("workload",  "?"))

        pts_x = [0,
                 lat.get("p50_waiting_time", lat.get("p50", 0)),
                 lat.get("p95_waiting_time", lat.get("p95", 0)),
                 lat.get("p99_waiting_time", lat.get("p99", 0)),
                 lat.get("max_waiting_time", lat.get("p99", 0)) * 1.05]
        pts_y = [0, 0.50, 0.95, 0.99, 1.0]

        color = _PALETTE[i % len(_PALETTE)]
        ax.plot(pts_x, pts_y, "o-", color=color, linewidth=1.5,
                label=f"{sched} / {wload}", markersize=4)

        for xv, yv, lbl in zip(pts_x[1:4], pts_y[1:4], ["P50", "P95", "P99"]):
            ax.annotate(lbl, (xv, yv), textcoords="offset points",
                        xytext=(4, 4), fontsize=7, color=color)

    ax.set_xlabel("Waiting Time (ticks)")
    ax.set_ylabel("CDF  P(W <= x)")
    ax.set_title(args.title or "Waiting-Time CDF (approximated from percentiles)", pad=12)
    ax.yaxis.set_major_formatter(ticker.PercentFormatter(xmax=1.0))
    ax.legend(fontsize=9)
    ax.grid(linewidth=0.4, alpha=0.5)

    plt.tight_layout()
    out = os.path.join(args.output, f"latency_cdf.{args.format}")
    fig.savefig(out, bbox_inches="tight")
    print(f"[plot_latency] Saved -> {out}")
    plt.close(fig)


if __name__ == "__main__":
    plot_latency(parse_args())
