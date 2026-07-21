# run_verification.ps1
# Phase 0 Regression Verification Pipeline

$ErrorActionPreference = "Stop"

Write-Host "========== Phase 0: Regression Verification =========="
Write-Host "Building Simulator and Test Suite..."

.\build_simulator.ps1

Write-Host "`n--- Running GoogleTest Suite ---"
Push-Location build_release
ctest --output-on-failure -C Release
$ctestResult = $LASTEXITCODE
Pop-Location

if ($ctestResult -ne 0) {
    Write-Error "GoogleTest Suite failed! Aborting pipeline."
    exit 1
}

Write-Host "`n--- Running embi_verify Theorem Checks ---"
$verifyCmd = "./build_release/bin/embi_verify.exe"
Invoke-Expression $verifyCmd
if ($LASTEXITCODE -ne 0) {
    Write-Error "embi_verify Theorem Checks failed! Aborting pipeline."
    exit 1
}

Write-Host "`nPhase 0 Verification Passed! The simulator is mathematically verified."
exit 0
