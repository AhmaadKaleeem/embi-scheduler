# Experiments Guide

## Exp 1: Symmetric Validation
Validates that EMBI and MaxWeight produce statistically identical tail latencies when process arrival rates are equal.
Command: `.\Event-driven-simulator-C++\build_release\bin\embi_sim.exe --experiment Event-driven-simulator-C++\examples\exp1_symmetric.json`

## Exp 2: Asymmetric Sweep
Evaluates scheduler performance when arrival rates are heterogeneous. Measures EMBI tail latency reduction.
Command: `.\Event-driven-simulator-C++\build_release\bin\embi_sim.exe --experiment Event-driven-simulator-C++\examples\exp2_asymmetric.json`

## Exp 3: High Contention
Stress tests the schedulers with a single highly contested lock.
Command: `.\Event-driven-simulator-C++\build_release\bin\embi_sim.exe --experiment Event-driven-simulator-C++\examples\exp3_high_contention.json`

## Exp 4: Long Hold Times
Evaluates sensitivity to critical section length.
Command: `.\Event-driven-simulator-C++\build_release\bin\embi_sim.exe --experiment Event-driven-simulator-C++\examples\exp4_long_hold.json`

## Exp 5: Scalability
Evaluates CPU overhead as process counts increase to 1024.
Command: `.\Event-driven-simulator-C++\build_release\bin\embi_sim.exe --experiment Event-driven-simulator-C++\examples\exp5_scalability.json`
