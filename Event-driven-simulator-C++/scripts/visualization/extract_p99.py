import json
from collections import defaultdict
import sys

import argparse

parser = argparse.ArgumentParser()
parser.add_argument('--input', type=str, required=True)
parser.add_argument('--output', type=str)
args = parser.parse_args()

with open(args.input, 'r') as f:
    data = json.load(f)

# Structure: spread -> scheduler -> list of p99s
results = defaultdict(lambda: defaultdict(list))

for run in data:
    sched = run['scheduler']
    if sched not in ['embi', 'maxweight']:
        continue
        
    spread = tuple(run.get('arrival_rate_asymmetric', []))
    if not spread:
        continue
        
    results[spread][sched].append(run['p99'])

print("EMBI vs MaxWeight P99 by Lambda Spread")
print("======================================")
for spread, scheds in results.items():
    embi_p99s = scheds.get('embi', [])
    mw_p99s = scheds.get('maxweight', [])
    
    if not embi_p99s or not mw_p99s:
        continue
        
    avg_embi = sum(embi_p99s) / len(embi_p99s)
    avg_mw = sum(mw_p99s) / len(mw_p99s)
    
    gain = ((avg_mw - avg_embi) / avg_mw) * 100 if avg_mw > 0 else 0
    
    print(f"Spread {spread}:")
    print(f"  EMBI P99:      {avg_embi:.1f}")
    print(f"  MaxWeight P99: {avg_mw:.1f}")
    print(f"  EMBI Gain:     {gain:+.1f}%")
    print()
