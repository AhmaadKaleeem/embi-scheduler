import matplotlib.pyplot as plt
import pandas as pd
import argparse
import numpy as np

def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--input', type=str, required=True)
    parser.add_argument('--output', type=str, required=True)
    return parser.parse_args()

def main():
    args = parse_args()
    try:
        df = pd.read_csv(args.input)
        
        fig, axes = plt.subplots(1, 2, figsize=(12, 5))
        
        axes[0].hist(df['tick'], bins=50, color='#4A90E2', edgecolor='black')
        axes[0].set_title('Event Arrivals over Time')
        axes[0].set_xlabel('Tick')
        axes[0].set_ylabel('Number of Events')
        axes[0].grid(True, linestyle='--', alpha=0.7)
        
        axes[1].hist(df['latency'], bins=50, color='#E24A4A', edgecolor='black')
        axes[1].set_title('Task Service Time Distribution')
        axes[1].set_xlabel('Latency')
        axes[1].set_ylabel('Count')
        axes[1].grid(True, linestyle='--', alpha=0.7)
        
        plt.tight_layout()
        plt.savefig(args.output, format='pdf', bbox_inches='tight')
        print(f"Generated trace stats graph: {args.output}")
    except Exception as e:
        print(f"Error plotting trace stats: {e}")

if __name__ == "__main__":
    main()
