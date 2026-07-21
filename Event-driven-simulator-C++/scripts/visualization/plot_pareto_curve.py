import json
import matplotlib.pyplot as plt
from collections import defaultdict
import argparse
import os

def parse_args():
    parser = argparse.ArgumentParser(description="Plot EMBI vs MaxWeight Pareto curve across lambda spreads.")
    parser.add_argument('--input', type=str, required=True, help="Path to summary.json")
    parser.add_argument('--output', type=str, required=True, help="Output directory for the plot")
    return parser.parse_args()

def main():
    args = parse_args()
    
    with open(args.input, 'r') as f:
        data = json.load(f)

    # spread -> scheduler -> list of p99s
    results = defaultdict(lambda: defaultdict(list))
    
    for run in data:
        sched = run.get('scheduler')
        if sched not in ['embi', 'maxweight']:
            continue
            
        spread = run.get('arrival_rate_asymmetric')
        if not spread:
            continue
            
        # Convert list to tuple for dictionary key
        spread_tuple = tuple(spread)
        results[spread_tuple][sched].append(run['p99'])
        
    x_labels = []
    embi_gains = []
    
    # Sort spreads by the first element (degree of skew)
    sorted_spreads = sorted(results.keys(), key=lambda s: s[0], reverse=True)
    
    for spread in sorted_spreads:
        scheds = results[spread]
        embi_p99s = scheds.get('embi', [])
        mw_p99s = scheds.get('maxweight', [])
        
        if not embi_p99s or not mw_p99s:
            continue
            
        avg_embi = sum(embi_p99s) / len(embi_p99s)
        avg_mw = sum(mw_p99s) / len(mw_p99s)
        
        # Gain is how much lower EMBI's P99 is compared to MaxWeight
        # E.g., MaxWeight=100, EMBI=60 -> 40% gain
        gain = ((avg_mw - avg_embi) / avg_mw) * 100 if avg_mw > 0 else 0
        
        label = f"{spread[0]:.2f} vs {spread[-1]:.2f}"
        if spread[0] == spread[-1]:
            label = "Symmetric"
            
        x_labels.append(label)
        embi_gains.append(gain)
        
        print(f"Spread {spread}: EMBI={avg_embi:.1f}, MaxWeight={avg_mw:.1f}, Gain={gain:+.1f}%")

    if not x_labels:
        print("No asymmetric spread data found in summary.json")
        return

    plt.figure(figsize=(10, 6))
    
    # Plot bars: green for positive gain, red for negative
    colors = ['#2ca02c' if g >= 0 else '#d62728' for g in embi_gains]
    bars = plt.bar(x_labels, embi_gains, color=colors, edgecolor='black', alpha=0.8)
    
    # Add a horizontal line at 0
    plt.axhline(0, color='black', linewidth=1.2, linestyle='--')
    
    plt.title("EMBI Predictive Advantage across $\lambda$ Spreads", fontsize=14, pad=15)
    plt.ylabel("EMBI Tail Latency Gain vs MaxWeight (%)", fontsize=12)
    plt.xlabel("Arrival Rate Spread (Heavy Hitter vs Light Hitter)", fontsize=12)
    plt.xticks(rotation=45, ha='right')
    
    # Add value labels on top of bars
    for bar in bars:
        yval = bar.get_height()
        offset = 2 if yval >= 0 else -6
        plt.text(bar.get_x() + bar.get_width()/2, yval + offset, 
                 f"{yval:+.1f}%", ha='center', va='bottom' if yval >= 0 else 'top',
                 fontsize=10, fontweight='bold')
                 
    plt.grid(axis='y', linestyle='--', alpha=0.5)
    plt.tight_layout()
    
    os.makedirs(args.output, exist_ok=True)
    out_file = os.path.join(args.output, "embi_pareto_curve.png")
    plt.savefig(out_file, dpi=300, bbox_inches='tight')
    print(f"Saved Pareto curve to {out_file}")

if __name__ == '__main__':
    main()
