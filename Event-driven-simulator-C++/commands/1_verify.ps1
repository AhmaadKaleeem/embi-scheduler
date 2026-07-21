# 1_verify.ps1
# Phase 0 & 1: Verify the Mathematics

$ErrorActionPreference = "Stop"

Write-Host "========== Phase 0 & 1: Regression Verification =========="
Write-Host "Building Simulator and running GoogleTest + Theorem checks."

# Execute from the project root
Push-Location "$PSScriptRoot\.."
.\run_verification.ps1
Pop-Location

Write-Host "`nVerification Complete!" -ForegroundColor Green
