# prepare_traces.ps1
# Trace Data Processing Automation Pipeline for Windows

$ErrorActionPreference = "Stop"

Write-Host "======================================"
Write-Host " Trace Data Processing Pipeline"
Write-Host "======================================"

# Verify Python 3 is installed
if (-not (Get-Command python -ErrorAction SilentlyContinue) -and -not (Get-Command python3 -ErrorAction SilentlyContinue)) {
    Write-Host "Error: Python is required but not installed." -ForegroundColor Red
    exit 1
}

$PythonCmd = if (Get-Command py -ErrorAction SilentlyContinue) { "py" } elseif (Get-Command python -ErrorAction SilentlyContinue) { "python" } else { "python3" }

# Verify dataset directory exists
$DATASET_DIR = "..\Dataset"
if (-not (Test-Path $DATASET_DIR)) {
    Write-Host "Warning: Expected dataset directory at $DATASET_DIR not found." -ForegroundColor Yellow
    Write-Host "Please ensure the raw traces (Azure, Alibaba, Google) are located in $DATASET_DIR"
}

Write-Host "Initiating Profiler..."
# Set PYTHONPATH so the profiler can resolve its modules correctly
$env:PYTHONPATH = "$PWD\scripts;" + $env:PYTHONPATH

# Run the trace profiler pipeline
& $PythonCmd -m trace_profiler.main

Write-Host "Building canonical CSVs from raw datasets..."
& $PythonCmd build_canonical.py

Write-Host "Generating trace stats graphs..."
$PDF_DIR = "results\pdf_figures"
if (-not (Test-Path $PDF_DIR)) {
    New-Item -ItemType Directory -Force -Path $PDF_DIR | Out-Null
}
foreach ($trace in @("google", "azure", "alibaba")) {
    $TRACE_FILE = "scripts\trace_profiler\output_reports\$trace\canonical.csv"
    if (Test-Path $TRACE_FILE) {
        & $PythonCmd scripts\visualization\plot_trace_stats.py --input $TRACE_FILE --output "$PDF_DIR\${trace}_trace_stats.pdf"
    }
}

Write-Host "======================================"
Write-Host " Trace Preparation Complete."
Write-Host " Canonical outputs and execution manifests are located in scripts\trace_profiler\output_reports\"
Write-Host " The canonical CSV can now be ingested by embi_sim."
Write-Host "======================================"
