import json, os, numpy as np
def summarize(exp):
    path = f'results/{exp}/summary.json'
    if not os.path.exists(path): return f'{exp}: No data'
    with open(path) as f: data = json.load(f)
    scheds = {}
    for d in data:
        s = d['scheduler']
        if s not in scheds: scheds[s] = {'p99':[], 'jain':[]}
        scheds[s]['p99'].append(d['p99'])
        scheds[s]['jain'].append(d['jain'])
    res = f'### {exp}\n'
    for s, v in scheds.items():
        res += f'- **{s}**: P99={np.median(v["p99"]):.1f}, Jain={np.median(v["jain"]):.3f}\n'
    return res

for e in ['exp1_symmetric', 'exp2_asymmetric', 'exp3_high_contention', 'exp4_long_hold', 'exp5_scalability']:
    print(summarize(e))
