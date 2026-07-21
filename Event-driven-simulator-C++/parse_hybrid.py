import os
import json
import pandas as pd
from glob import glob

base_dir = "results/hybrid_noise/hybrid_embi/poisson"
csvs = glob(os.path.join(base_dir, "*", "run.csv"))
results = []

for csv in csvs:
    print(f"Processing {csv}...")
    dir_path = os.path.dirname(csv)
    with open(os.path.join(dir_path, "summary.json"), 'r') as f:
        summary = json.load(f)
        noise = summary['config']['lambda_noise_stddev']
    
    # Run the diagnostics extraction logic inline
    df = pd.read_csv(csv)
    chosen_df = df[df['chosen'] == 1].copy()
    
    if 'branch' in chosen_df.columns and chosen_df['branch'].max() > 0:
        fallback_pct = float((chosen_df['branch'] == 1).mean() * 100)
        embi_pct = float((chosen_df['branch'] == 2).mean() * 100)
        mean_gap = float(chosen_df['gap'].mean())
        mean_tau = float(chosen_df['tau'].mean())
        
        tau_nonzero = chosen_df[chosen_df['tau'] > 0]
        mean_gap_tau = float((tau_nonzero['gap'] / tau_nonzero['tau']).mean()) if len(tau_nonzero) > 0 else 0.0
        
        branches = chosen_df['branch'].values
        transitions = int(np.sum(branches[1:] != branches[:-1])) if len(branches) > 0 else 0
        
        results.append({
            'NoiseLevel': noise,
            'FallbackPct': fallback_pct,
            'EMBIPct': embi_pct,
            'MeanGap': mean_gap,
            'MeanTau': mean_tau,
            'Transitions': transitions
        })

df_res = pd.DataFrame(results)
df_res = df_res.sort_values('NoiseLevel')
print("\n--- Hybrid Noise Sweep Results ---")
print(df_res.to_string(index=False))
df_res.to_csv("results/hybrid_noise_summary.csv", index=False)
