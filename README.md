# EMBI Scheduler: Estimated Marginal Blocking Impact

> **Research in Progress:** Simulator code for an upcoming systems paper submission.

EMBI is an OS scheduler derived from minimizing the one-step Lyapunov drift of a quadratic contention potential. It mitigates lock contention by using arrival rate predictions to preempt current lock holders.

**Formula:**
`Φ_i = μ_i * (2Q_i + 2λ_i - M)`

## Quick Start
```bash
cd Event-driven-simulator-C++
cmake -S . -B build_release -DCMAKE_BUILD_TYPE=Release
cmake --build build_release -j
.\build_release\bin\embi_sim.exe --experiment examples\exp1_symmetric.json
```

## Results
Refer to the `figs/` directory for generated plots showing EMBI latency reduction.

## Documentation
* [Architecture](docs/architecture.md)
* [Experiments](docs/experiments.md)
* [Theory Companion](docs/theory.md)

## License
GPLv2
