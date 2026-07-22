import os
import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns

def main():
    print("Generating figures...")
    data_file = "results/aggregated/summary.csv"
    if not os.path.exists(data_file):
        print("Data file not found. Run aggregate.py first.")
        return

    df = pd.read_csv(data_file)
    os.makedirs("results/paper/figures", exist_ok=True)
    
    # Setup aesthetic style
    sns.set_theme(style="whitegrid", palette="deep")
    plt.rcParams.update({
        'font.size': 12,
        'axes.labelsize': 14,
        'axes.titlesize': 16,
        'legend.fontsize': 12,
        'xtick.labelsize': 12,
        'ytick.labelsize': 12,
        'figure.dpi': 300
    })

    # Plot 1: Tail Latency vs Utilization (Arrival Rate) for Mechanism Validation
    exp_data = df[df['experiment'] == 'RQ2_Latency']
    if not exp_data.empty:
        plt.figure(figsize=(10, 6))
        for wl in exp_data['workload'].unique():
            wl_data = exp_data[exp_data['workload'] == wl]
            sns.lineplot(data=wl_data, x='arrival_rate', y='p99_mean', hue='scheduler', marker='o')
            plt.title(f'Mechanism Validation: 99th Percentile Latency vs Utilization ({wl})')
            plt.xlabel('Arrival Rate (Utilization)')
            plt.ylabel('P99 Latency (ticks)')
            plt.yscale('log')
            plt.tight_layout()
            plt.savefig(f"results/paper/figures/mechanism_p99_{wl}.pdf")
            plt.close()

    # Plot 2: Causal Chain (Prediction Error vs Ranking Divergence)
    if 'lambda_mse_mean' in df.columns and 'mw_divergence_mean' in df.columns:
        plt.figure(figsize=(8, 6))
        sns.scatterplot(data=df, x='lambda_mse_mean', y='mw_divergence_mean', hue='scheduler', style='workload')
        plt.title('Causal Chain: Estimator Error vs Rank Divergence')
        plt.xlabel('Lambda MSE')
        plt.ylabel('Fraction of Decisions Differing from MaxWeight')
        plt.tight_layout()
        plt.savefig("results/paper/figures/causal_chain.pdf")
        plt.close()

    # Plot 3: Fairness vs Latency Pareto
    plt.figure(figsize=(8, 6))
    sns.scatterplot(data=df, x='p99_mean', y='jain_mean', hue='scheduler', style='workload', s=100)
    plt.title('Fairness-Latency Pareto Frontier')
    plt.xlabel('P99 Latency (ticks)')
    plt.ylabel("Jain's Fairness Index")
    plt.xscale('log')
    plt.tight_layout()
    plt.savefig("results/paper/figures/fairness_pareto.pdf")
    plt.close()
    
    print("Figures generated in results/paper/figures/")

if __name__ == "__main__":
    main()
