import json, os, numpy as np

def summarize(exp):
    path = f'results/{exp}/summary.json'
    if not os.path.exists(path): return f'{exp}: No data'
    with open(path) as f: data = json.load(f)
    scheds = {}
    for d in data:
        s = d['scheduler']
        if s not in scheds: 
            scheds[s] = {'p99':[], 'p50':[], 'avg_wait':[], 'jain':[], 'starve':[], 'util':[], 'tput':[]}
        scheds[s]['p99'].append(d.get('p99', 0))
        scheds[s]['p50'].append(d.get('p50', 0))
        scheds[s]['avg_wait'].append(d.get('avg_waiting_time', 0))
        scheds[s]['jain'].append(d.get('jain', 0))
        scheds[s]['starve'].append(d.get('max_starvation', 0))
        scheds[s]['util'].append(d.get('utilization', 0))
        scheds[s]['tput'].append(d.get('throughput', 0))
        
    res = f'### {exp}\n'
    for s, v in scheds.items():
        if s in ['embi', 'maxweight']:
            res += f'- **{s.upper()}**:\n'
            res += f'  - P99 Latency: {np.median(v["p99"]):.1f} ticks\n'
            res += f'  - P50 Latency: {np.median(v["p50"]):.1f} ticks\n'
            res += f'  - Avg Wait: {np.median(v["avg_wait"]):.1f} ticks\n'
            res += f'  - Jain Fairness: {np.median(v["jain"]):.4f}\n'
            res += f'  - Max Starvation: {np.median(v["starve"]):.1f} ticks\n'
            res += f'  - CPU Util: {np.median(v["util"])*100:.2f}%\n'
            res += f'  - Throughput: {np.median(v["tput"]):.2f} req/tick\n'
    return res

for e in ['exp1_symmetric', 'exp2_asymmetric', 'exp3_high_contention', 'exp4_long_hold', 'exp5_scalability']:
    print(summarize(e))
