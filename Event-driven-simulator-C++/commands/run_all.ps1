# run_all.ps1
# Master Script: Executes the entire EMBI pipeline

param (
    [switch]$SkipVerify,
    [switch]$SkipBenchmark,
    [switch]$SkipFigures
)

$ErrorActionPreference = "Stop"

Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host " EMBI SIMULATOR FULL PIPELINE EXECUTION" -ForegroundColor Cyan
Write-Host "==========================================================" -ForegroundColor Cyan

$scriptPath = $PSScriptRoot

if (-not $SkipVerify) {
    Write-Host "`n>>> Starting Phase 1: Verification" -ForegroundColor Yellow
    & "$scriptPath\1_verify.ps1"
    if (-not $?) { Write-Error "Phase 1: Verification failed! Aborting."; exit 1 }
} else {
    Write-Host "`n>>> Skipping Phase 1: Verification" -ForegroundColor DarkGray
}

if (-not $SkipBenchmark) {
    Write-Host "`n>>> Starting Phase 2: Benchmarking" -ForegroundColor Yellow
    & "$scriptPath\2_benchmark.ps1"
    if (-not $?) { Write-Error "Phase 2: Benchmarking failed! Aborting."; exit 1 }
} else {
    Write-Host "`n>>> Skipping Phase 2: Benchmarking" -ForegroundColor DarkGray
}

if (-not $SkipFigures) {
    Write-Host "`n>>> Starting Phase 3: Analytics and Figure Generation" -ForegroundColor Yellow
    & "$scriptPath\3_generate_figures.ps1"
    if (-not $?) { Write-Error "Phase 3: Generation failed! Aborting."; exit 1 }
} else {
    Write-Host "`n>>> Skipping Phase 3: Analytics and Figure Generation" -ForegroundColor DarkGray
}

Write-Host "`n==========================================================" -ForegroundColor Cyan
Write-Host " PIPELINE SUCCESSFULLY COMPLETED!" -ForegroundColor Green
Write-Host "==========================================================" -ForegroundColor Cyan
