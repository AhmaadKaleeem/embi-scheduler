#!/usr/bin/env python3
import os
import pandas as pd
import matplotlib.pyplot as plt
import json

RAW_DIR = "results/raw"
PLOTS_DIR = "results/paper/plots"
TABLES_DIR = "results/paper/tables"

def ensure_dirs():
    os.makedirs(PLOTS_DIR, exist_ok=True)
    os.makedirs(TABLES_DIR, exist_ok=True)

def plot_hybrid_gate():
    # Find a hybrid run
    hybrid_path = os.path.join(RAW_DIR, "RQ5_Hybrid_Resolution", "uniform", "hybrid_embi", "seed_1", "hybrid_trace.csv")
    if not os.path.exists(hybrid_path):
        print(f"Skipping hybrid gate plot: {hybrid_path} not found.")
        return

    df = pd.read_csv(hybrid_path)
    if df.empty:
        return

    plt.figure(figsize=(10, 6))
    plt.plot(df['tick'], df['gap'], label='g_hat(X)', color='blue', alpha=0.7)
    plt.plot(df['tick'], df['tau'], label='tau(X)', color='red', linestyle='--')
    
    # Shade regions where gap <= tau (Fallback)
    plt.fill_between(df['tick'], 0, df['tau'], where=(df['gap'] <= df['tau']), color='gray', alpha=0.3, label='Fallback Region')
    
    # Shade regions where gap > tau (EMBI)
    plt.fill_between(df['tick'], df['tau'], df['gap'], where=(df['gap'] > df['tau']), color='green', alpha=0.3, label='EMBI Region')

    plt.xlabel('Simulation Ticks')
    plt.ylabel('Score Gap')
    plt.title('Hybrid Confidence Gate: g_hat(X) vs tau(X)')
    plt.legend()
    plt.tight_layout()
    plt.savefig(os.path.join(PLOTS_DIR, "figure4_gate.pdf"))
    plt.close()
    print("Generated figure4_gate.pdf")

def generate_latex_table():
    # Simple latex table generation example
    table_path = os.path.join(TABLES_DIR, "table1.tex")
    with open(table_path, "w") as f:
        f.write("\\begin{table}[h]\n")
        f.write("\\centering\n")
        f.write("\\begin{tabular}{|l|c|c|c|}\n")
        f.write("\\hline\n")
        f.write("Scheduler & Throughput & P99 Latency & Jain Index \\\\\n")
        f.write("\\hline\n")
        f.write("FCFS & 0.95 & 120.5 & 0.99 \\\\\n")
        f.write("MaxWeight & 0.98 & 45.2 & 0.98 \\\\\n")
        f.write("EMBI & 0.99 & 15.1 & 0.99 \\\\\n")
        f.write("\\hline\n")
        f.write("\\end{tabular}\n")
        f.write("\\caption{Placeholder for aggregated results}\n")
        f.write("\\end{table}\n")
    print("Generated table1.tex (placeholder)")

def main():
    ensure_dirs()
    print("========== Generating Publication Figures ==========")
    plot_hybrid_gate()
    generate_latex_table()
    print("Plotting complete.")

if __name__ == "__main__":
    main()
