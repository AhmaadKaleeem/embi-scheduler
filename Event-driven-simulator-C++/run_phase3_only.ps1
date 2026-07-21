$ErrorActionPreference = "Stop"

Write-Host "======================================"
Write-Host " PHASE 3: TRACE-DRIVEN BENCHMARKS"
Write-Host "======================================"

$EMBI_SIM = ""
if (Test-Path ".\build_release\Release\embi_sim.exe") {
    $EMBI_SIM = ".\build_release\Release\embi_sim.exe"
} elseif (Test-Path ".\build_release\bin\embi_sim.exe") {
    $EMBI_SIM = ".\build_release\bin\embi_sim.exe"
} elseif (Test-Path ".\build_release\embi_sim.exe") {
    $EMBI_SIM = ".\build_release\embi_sim.exe"
} else {
    Write-Host "Error: embi_sim.exe not found! Please run .\run_pipeline.ps1 first to build it." -ForegroundColor Red
    exit 1
}

$TRACE_OUTPUT_DIR = "scripts\trace_profiler\output_reports"
$TRACES = @("google", "azure", "alibaba")

foreach ($trace_name in $TRACES) {
    $TRACE_FILE = "$TRACE_OUTPUT_DIR\$trace_name\canonical.csv"
    $RESULT_DIR = "results\trace_eval_$trace_name"
    
    Write-Host "--------------------------------------"
    if (Test-Path $TRACE_FILE) {
        Write-Host "Running trace evaluation for: $trace_name"
        & $EMBI_SIM --scheduler embi --workload trace --trace "$TRACE_FILE" --output "$RESULT_DIR"
        Write-Host "Saved results to: $RESULT_DIR"
        
        $PythonCmd = ""
        if (Get-Command py -ErrorAction SilentlyContinue) { $PythonCmd = "py" }
        elseif (Get-Command python -ErrorAction SilentlyContinue) { 
            $test = & python -V 2>&1
            if ("$test" -match "Python") { $PythonCmd = "python" }
        }
        elseif (Get-Command python3 -ErrorAction SilentlyContinue) { $PythonCmd = "python3" }
        if ($PythonCmd -and (Test-Path "$RESULT_DIR\run.csv")) {
            $PDF_DIR = "results\pdf_figures"
            if (-not (Test-Path $PDF_DIR)) { New-Item -ItemType Directory -Force -Path $PDF_DIR | Out-Null }
            & $PythonCmd scripts\visualization\plot_trace_results.py --input "$RESULT_DIR\run.csv" --output "$PDF_DIR\${trace_name}_sim_results.pdf"
        }
    } else {
        Write-Host "Skipping $($trace_name): Canonical trace file not found at $TRACE_FILE." -ForegroundColor Yellow
        Write-Host "Make sure you have run .\prepare_traces.ps1" -ForegroundColor Yellow
    }
}

Write-Host "======================================"
Write-Host " Phase 3 Complete."
Write-Host "======================================"
