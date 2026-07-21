import matplotlib.pyplot as plt
import pandas as pd
import argparse
import numpy as np
import os

def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--input', type=str, required=True)
    parser.add_argument('--output', type=str, required=True)
    return parser.parse_args()

def main():
    args = parse_args()
    try:
        if not os.path.exists(args.input):
            print(f"Skipping plot: File not found {args.input}")
            return
            
        df = pd.read_csv(args.input)
        if 'completion_time' not in df.columns:
            print(f"Skipping plot: No completion_time in {args.input}")
            return
            
        df_completed = df[df['completion_time'] > 0]
        
        fig, ax = plt.subplots(figsize=(8, 6))
        
        if not df_completed.empty:
            wait_times = np.sort(df_completed['waiting_time'].values)
            y = np.arange(1, len(wait_times)+1) / len(wait_times)
            ax.plot(wait_times, y, marker='.', linestyle='none', color='#4A90E2', markersize=2)
            ax.set_title('CDF of Task Waiting Times')
            ax.set_xlabel('Waiting Time')
            ax.set_ylabel('CDF')
            ax.grid(True, linestyle='--', alpha=0.7)
            
            # Add median and p99 lines
            if len(wait_times) > 0:
                p50 = np.percentile(wait_times, 50)
                p99 = np.percentile(wait_times, 99)
                ax.axvline(p50, color='green', linestyle=':', label=f'P50: {p50:.2f}')
                ax.axvline(p99, color='red', linestyle=':', label=f'P99: {p99:.2f}')
                ax.legend()
        else:
            ax.text(0.5, 0.5, 'No completed tasks found in trace.', ha='center')
            
        plt.tight_layout()
        plt.savefig(args.output, format='pdf', bbox_inches='tight')
        print(f"Generated trace results graph: {args.output}")
    except Exception as e:
        print(f"Error plotting trace results: {e}")

if __name__ == "__main__":
    main()
