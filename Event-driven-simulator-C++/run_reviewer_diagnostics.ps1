$ErrorActionPreference = "Stop"

Write-Host "Running Heavy Load (RQ1) diagnostic..."
.\build_release\bin\embi_sim.exe --ticks 1000000 --scheduler embi --arrival-rate 0.5 --log-freq 1 --output results/diag_heavy
python scripts/expanded_diagnostics.py results/diag_heavy/run.csv > results/diag_heavy/diagnostics.json

Write-Host "Running Low Load (RQ7) diagnostic..."
.\build_release\bin\embi_sim.exe --ticks 1000000 --scheduler embi --arrival-rate 0.05 --log-freq 1 --output results/diag_low
python scripts/expanded_diagnostics.py results/diag_low/run.csv > results/diag_low/diagnostics.json

Write-Host "Running Hybrid (RQ5 Asymmetric) diagnostic..."
.\build_release\bin\embi_sim.exe --ticks 1000000 --scheduler hybrid_embi --arrival-rate 0.5 --log-freq 1 --output results/diag_hybrid --config config.default.yaml
python scripts/expanded_diagnostics.py results/diag_hybrid/run.csv > results/diag_hybrid/diagnostics.json

Write-Host "Diagnostics generated."
