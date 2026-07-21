# 3_generate_figures.ps1
# Phase 3: Generate the Publication Figures

$ErrorActionPreference = "Stop"

Write-Host "========== Phase 3: Generating Publication Figures =========="
Write-Host "Running Python analytical scripts to ensure data integrity and generate plots."

# Execute from the project root
Push-Location "$PSScriptRoot\.."
python .\scripts\sanity_checks.py
if ($LASTEXITCODE -ne 0) { Write-Error "Sanity checks failed!"; exit 1 }

python .\scripts\generate_figures.py
if ($LASTEXITCODE -ne 0) { Write-Error "Figure generation failed!"; exit 1 }
Pop-Location

Write-Host "`nFigures and Tables Complete! Saved in results/paper/" -ForegroundColor Green
