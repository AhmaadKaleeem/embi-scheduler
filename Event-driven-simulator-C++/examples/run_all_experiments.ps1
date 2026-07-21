$ErrorActionPreference = "Stop"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Definition
$RepoRoot = Split-Path -Parent $ScriptDir
$Bin = "$RepoRoot\build_release\bin\embi_sim.exe"
$Figs = "$RepoRoot\figs"
$Python = "python"

if (-Not (Test-Path -Path $Bin -PathType Leaf)) {
    Write-Host "[ERROR] embi_sim not found at: $Bin" -ForegroundColor Red
    Write-Host "        You must build the project first!" -ForegroundColor Yellow
    Write-Host "        Run: cmake -S . -B build_release -DCMAKE_BUILD_TYPE=Release" -ForegroundColor Yellow
    Write-Host "        Run: cmake --build build_release -j" -ForegroundColor Yellow
    exit 1
}

Write-Host "Running Exp 1: Symmetric Validation..." -ForegroundColor Cyan
& $Bin --experiment "$ScriptDir\exp1_symmetric.json"

Write-Host "Running Exp 2: Asymmetric Sweep..." -ForegroundColor Cyan
& $Bin --experiment "$ScriptDir\exp2_asymmetric.json"

Write-Host "Running Exp 3: High Contention..." -ForegroundColor Cyan
& $Bin --experiment "$ScriptDir\exp3_high_contention.json"

Write-Host "Running Exp 4: Long Hold Times..." -ForegroundColor Cyan
& $Bin --experiment "$ScriptDir\exp4_long_hold.json"

Write-Host "Running Exp 5: Scalability..." -ForegroundColor Cyan
& $Bin --experiment "$ScriptDir\exp5_scalability.json"

Write-Host "Running Exp 6: Confidence-Gated Hybrid Fallback..." -ForegroundColor Cyan
& $Bin --experiment "$ScriptDir\exp6_confidence_hybrid.json"

Write-Host "Generating PDF Figures..." -ForegroundColor Cyan
& $Python "$RepoRoot\scripts\compare_schedulers.py" --input "$RepoRoot\results\exp2_asymmetric\summary.json" --output $Figs --format pdf
& $Python "$RepoRoot\scripts\plot_pareto_curve.py" --input "$RepoRoot\results\exp2_asymmetric\summary.json" --output $Figs
& $Python "$RepoRoot\scripts\plot_fairness.py" --input "$RepoRoot\results\exp2_asymmetric\summary.json" --output $Figs --format pdf
& $Python "$RepoRoot\scripts\plot_latency.py" --input "$RepoRoot\results\exp3_high_contention\summary.json" --output $Figs --format pdf

Write-Host "All experiments and plotting complete! Check the figs/ folder." -ForegroundColor Green
