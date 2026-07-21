import pandas as pd
import numpy as np
import argparse
import json

def compute_diagnostics(csv_path):
    df = pd.read_csv(csv_path)
    
    # 1. Queue Statistics
    q_mean = float(df['queue'].mean())
    q_median = float(df['queue'].median())
    q_95 = float(df['queue'].quantile(0.95))
    q_99 = float(df['queue'].quantile(0.99))
    q_max = float(df['queue'].max())
    
    # 2. Score Statistics (for chosen process, or all?)
    # The reviewer wants mean(raw score) for EMBI. Let's filter for chosen == 1
    chosen_df = df[df['chosen'] == 1].copy()
    
    if 'raw_score' in chosen_df.columns and not chosen_df['raw_score'].isnull().all():
        s_mean = float(chosen_df['raw_score'].mean())
        s_std = float(chosen_df['raw_score'].std())
        s_min = float(chosen_df['raw_score'].min())
        s_max = float(chosen_df['raw_score'].max())
    else:
        s_mean = s_std = s_min = s_max = 0.0

    # 3. Ratio
    # (2*lambda - M) / (2*Q)
    # Filter where Q > 0 to avoid division by zero
    q_nonzero = df[df['queue'] > 0].copy()
    if len(q_nonzero) > 0:
        ratio = (2.0 * q_nonzero['lambda_hat'] - 10.0) / (2.0 * q_nonzero['queue'])
        ratio_mean = float(ratio.mean())
    else:
        ratio_mean = 0.0
        
    # 4. Contribution analysis
    # Score = mu_hat * (2Q + 2lambda - M)
    # Queue term = 2 * mu_hat * Q
    # Lambda term = 2 * mu_hat * lambda
    # Penalty term = mu_hat * M
    if len(chosen_df) > 0:
        queue_term = (2.0 * chosen_df['mu_hat'] * chosen_df['queue']).abs().mean()
        lambda_term = (2.0 * chosen_df['mu_hat'] * chosen_df['lambda_hat']).abs().mean()
        penalty_term = (10.0 * chosen_df['mu_hat']).abs().mean()
        total = queue_term + lambda_term + penalty_term
        if total > 0:
            q_pct = float((queue_term / total) * 100)
            l_pct = float((lambda_term / total) * 100)
            p_pct = float((penalty_term / total) * 100)
        else:
            q_pct = l_pct = p_pct = 0.0
    else:
        q_pct = l_pct = p_pct = 0.0
        
    # 5. Hybrid stats
    hybrid_stats = {}
    if 'branch' in chosen_df.columns and chosen_df['branch'].max() > 0:
        # branch 1 = fallback (MW), branch 2 = EMBI
        fallback_pct = float((chosen_df['branch'] == 1).mean() * 100)
        embi_pct = float((chosen_df['branch'] == 2).mean() * 100)
        mean_gap = float(chosen_df['gap'].mean())
        mean_tau = float(chosen_df['tau'].mean())
        # gap/tau
        tau_nonzero = chosen_df[chosen_df['tau'] > 0]
        mean_gap_tau = float((tau_nonzero['gap'] / tau_nonzero['tau']).mean()) if len(tau_nonzero) > 0 else 0.0
        
        # Transition count and streaks
        branches = chosen_df['branch'].values
        transitions = np.sum(branches[1:] != branches[:-1])
        
        streaks = []
        current_streak = 1
        if len(branches) > 0:
            for i in range(1, len(branches)):
                if branches[i] == branches[i-1]:
                    current_streak += 1
                else:
                    streaks.append((branches[i-1], current_streak))
                    current_streak = 1
            streaks.append((branches[-1], current_streak))
        
        embi_streaks = [s[1] for s in streaks if s[0] == 2]
        mw_streaks = [s[1] for s in streaks if s[0] == 1]
        
        avg_embi_streak = np.mean(embi_streaks) if embi_streaks else 0.0
        avg_mw_streak = np.mean(mw_streaks) if mw_streaks else 0.0
        
        hybrid_stats = {
            'fallback_pct': fallback_pct,
            'embi_pct': embi_pct,
            'mean_gap': mean_gap,
            'mean_tau': mean_tau,
            'mean_gap_tau': mean_gap_tau,
            'transition_count': int(transitions),
            'avg_embi_streak': avg_embi_streak,
            'avg_mw_streak': avg_mw_streak
        }
        
    # 6. Context switches per million ticks
    pids = chosen_df['pid'].values
    context_switches = np.sum(pids[1:] != pids[:-1])
    total_ticks = df['tick'].max()
    cs_per_million = (context_switches / total_ticks) * 1e6 if total_ticks > 0 else 0.0
    
    results = {
        'Queue Statistics': {
            'mean': q_mean, 'median': q_median, '95th': q_95, '99th': q_99, 'max': q_max
        },
        'Score Statistics': {
            'mean': s_mean, 'std': s_std, 'min': s_min, 'max': s_max
        },
        'Ratio (2lambda-M)/(2Q)': ratio_mean,
        'Contribution Analysis': {
            'Queue %': q_pct,
            'Lambda %': l_pct,
            'Penalty %': p_pct
        },
        'Hybrid Statistics': hybrid_stats,
        'Context Switches / 1M Ticks': cs_per_million,
        'Raw Context Switches': int(context_switches)
    }
    
    print(json.dumps(results, indent=2))
    return results

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('csv_path', help='Path to simulation CSV log')
    args = parser.parse_args()
    compute_diagnostics(args.csv_path)
