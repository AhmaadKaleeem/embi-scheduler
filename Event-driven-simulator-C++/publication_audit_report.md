# EMBI Framework Publication Audit Report

This audit answers the core scientific questions based on the full benchmark suite execution.

## 1. Does EMBI reduce P99 latency?
For every workload, here is the comparison between FCFS/MaxWeight and EMBI.
| Workload | Scheduler | Mean Latency | P99 Latency | P99 Improvement | 95% CI | Significant? |
|---|---|---|---|---|---|---|
| bursty (Rate 0.1) | EMBI | 5.93 | 51.34 | -82.17% | ±4.76 | NO |
| bursty (Rate 0.01) | EMBI | 0.01 | 0.18 | 0.00% | ±0.05 | NO |
| heavy_tail (Rate 0.1) | EMBI | 318.24 | 1122.20 | -22.58% | ±208.22 | NO |
| heavy_tail (Rate 0.01) | EMBI | 0.04 | 0.75 | 0.00% | ±0.06 | NO |
| poisson (Rate 0.1) | EMBI | 10.25 | 69.00 | -43.32% | ±7.75 | NO |
| poisson (Rate 0.01) | EMBI | 0.04 | 0.99 | -32.00% | ±0.10 | NO |
| poisson (Rate 0.95) | EMBI | 9.55 | 147.87 | -14687.00% | ±1.39 | NO |
| uniform (Rate 0.1) | EMBI | 1.70 | 21.20 | -229.15% | ±2.21 | NO |
| uniform (Rate 0.01) | EMBI | 0.04 | 0.75 | 0.00% | ±0.06 | NO |
| human_trace (Rate 0.5) | EMBI | 0.00 | 0.00 | 0.00% | ±0.00 | NO |

## 8. Scheduler Cost
| Scheduler | Avg Runtime (ns) |
|---|---|
| cmu | 283.14 |
| embi | 490.07 |
| embi_oracle | 524.35 |
| fcfs | 218.92 |
| hybrid_embi | 723.12 |
| maxweight | 256.75 |
| rr | 217.06 |
| embi_unclipped | 758.85 |

## 10. Fairness
| Scheduler | Jain Index | 95% CI |
|---|---|---|
| cmu | 0.7988 | ±0.0003 |
| embi | 0.9056 | ±0.0004 |
| embi_oracle | 0.8323 | ±0.0002 |
| fcfs | 0.8846 | ±0.0005 |
| hybrid_embi | 0.7988 | ±0.0003 |
| maxweight | 0.8846 | ±0.0005 |
| rr | 0.8846 | ±0.0005 |
| embi_unclipped | 1.0000 | ±0.0000 |