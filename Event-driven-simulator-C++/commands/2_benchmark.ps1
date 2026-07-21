# 2_benchmark.ps1
# Phase 2: Run the Benchmarks

$ErrorActionPreference = "Stop"

Write-Host "========== Phase 2: Running Benchmarks =========="
Write-Host "Executing the exhaustive 50-seed sweeps across all research questions."
Write-Host "Note: This will take some time to complete (1,800+ simulations)."

# Execute from the project root
Push-Location "$PSScriptRoot\.."
.\run_golden_pipeline.ps1
Pop-Location

Write-Host "`nBenchmarks Complete! Data saved in results/raw/" -ForegroundColor Green
