$ErrorActionPreference = "Continue"

Write-Host "======================================"
Write-Host " Cleaning EMBI Simulator Workspace"
Write-Host "======================================"

$dirs_to_remove = @(
    "build_release",
    "results",
    "scripts\trace_profiler\output_reports"
)

foreach ($dir in $dirs_to_remove) {
    if (Test-Path $dir) {
        Write-Host "Removing $dir..."
        Remove-Item -Recurse -Force $dir
    }
}

# Clean __pycache__ directories
Get-ChildItem -Path . -Filter "__pycache__" -Recurse -Directory | Remove-Item -Recurse -Force

Write-Host "======================================"
Write-Host " Workspace is clean!"
Write-Host "======================================"
