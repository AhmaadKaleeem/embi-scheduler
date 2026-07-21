# =============================================================================
# run_lock_contention.ps1
#
# Runs the full sweep for the lock contention workload and generates plots.
#
# Usage:
#   .\examples\run_lock_contention.ps1
# =============================================================================
$ErrorActionPreference = "Stop"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Definition
$RepoRoot = Split-Path -Parent $ScriptDir
$Bin = "$RepoRoot\build_release\bin\embi_sim.exe"
$Results = "$RepoRoot\results"
$Figs = "$RepoRoot\figs"
$Python = "python"

if (-Not (Test-Path -Path $Bin -PathType Leaf)) {
    Write-Host "[ERROR] embi_sim not found at: $Bin" -ForegroundColor Red
    Write-Host "        Build first: cmake --build build_release --config Release" -ForegroundColor Yellow
    exit 1
}

$Timestamp = Get-Date -Format "HH:mm:ss"
Write-Host "[$Timestamp] Starting Lock Contention Sweep..." -ForegroundColor Cyan
Write-Host "[$Timestamp] This will run EMBI, MaxWeight, RR, and FCFS with asymmetric arrival rates."

# Run the sweep using the JSON configuration
& $Bin --experiment "$ScriptDir\lock_contention_sweep.json" | Tee-Object -FilePath "$Results\lock_contention.log"

$Timestamp = Get-Date -Format "HH:mm:ss"
Write-Host "[$Timestamp] Sweep complete. Generating plots..." -ForegroundColor Cyan

$SweepJson = "$Results\lock_contention_sweep\summary.json"

if (Test-Path -Path $SweepJson -PathType Leaf) {
    # Plot scheduler comparison (bar charts)
    & $Python "$RepoRoot\scripts\compare_schedulers.py" --input "$SweepJson" --output "$Figs\lock_contention_sweep"
    
    # Plot EMBI Pareto curve for lambda spreads
    & $Python "$RepoRoot\scripts\plot_pareto_curve.py" --input "$SweepJson" --output "$Figs\lock_contention_sweep"
    
    # Plot Fairness
    & $Python "$RepoRoot\scripts\plot_fairness.py" --input "$SweepJson" --output "$Figs\lock_contention_sweep\fairness"

    # Plot Latency CDFs and distributions
    & $Python "$RepoRoot\scripts\plot_latency.py" --input "$SweepJson" --output "$Figs\lock_contention_sweep\latency"
    
    $Timestamp = Get-Date -Format "HH:mm:ss"
    Write-Host "[$Timestamp] Plots generated in $Figs\lock_contention_sweep\" -ForegroundColor Green
} else {
    Write-Host "[ERROR] summary.json not found. Did the sweep fail?" -ForegroundColor Red
}

$Timestamp = Get-Date -Format "HH:mm:ss"
Write-Host "[$Timestamp] All done!" -ForegroundColor Cyan
