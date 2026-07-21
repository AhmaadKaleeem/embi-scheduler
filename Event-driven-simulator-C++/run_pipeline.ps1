# run_pipeline.ps1
# End-to-end execution pipeline for EMBI Scheduling Simulator on Windows

param(
    [string]$Scheduler = "embi"
)

$ErrorActionPreference = "Stop"

# Define directories
$PDF_DIR = "results\pdf_figures"
$TRACE_OUTPUT_DIR = "scripts\trace_profiler\output_reports"

Write-Host "======================================"
Write-Host " PHASE 1: SYNTHETIC BENCHMARKS"
Write-Host "======================================"
Write-Host " 1.1 Building Simulator (Release Mode)"
$CMakeGenerator = ""
if (Get-Command gcc -ErrorAction SilentlyContinue) {
    $CMakeGenerator = '-G "MinGW Makefiles"'
}
Invoke-Expression "cmake $CMakeGenerator -B build_release -DCMAKE_BUILD_TYPE=Release"
cmake --build build_release --config Release

$EMBI_SIM = ""
if (Test-Path ".\build_release\Release\embi_sim.exe") {
    $EMBI_SIM = ".\build_release\Release\embi_sim.exe"
} elseif (Test-Path ".\build_release\bin\embi_sim.exe") {
    $EMBI_SIM = ".\build_release\bin\embi_sim.exe"
} elseif (Test-Path ".\build_release\embi_sim.exe") {
    $EMBI_SIM = ".\build_release\embi_sim.exe"
} else {
    Write-Host "Error: embi_sim.exe not found after build! CMake configuration may have failed." -ForegroundColor Red
    exit 1
}

if (-not (Test-Path $PDF_DIR)) {
    New-Item -ItemType Directory -Force -Path $PDF_DIR | Out-Null
}

Write-Host "--------------------------------------"
Write-Host " 1.2 Executing 6 Synthetic Experiments using scheduler: $Scheduler"
$EXPERIMENTS = @(
    "exp1_symmetric.json",
    "exp2_asymmetric.json",
    "exp3_high_contention.json",
    "exp4_long_hold.json",
    "exp5_scalability.json",
    "exp6_confidence_hybrid.json"
)

foreach ($exp in $EXPERIMENTS) {
    Write-Host "Running synthetic experiment: $exp..."
    $expName = [System.IO.Path]::GetFileNameWithoutExtension($exp)
    $outputDir = "results\$expName"
    
    Write-Host "Running $exp with scheduler: $Scheduler" -ForegroundColor Cyan
    & $EMBI_SIM --scheduler $Scheduler --experiment "examples\$exp" --output $outputDir
}

Write-Host "--------------------------------------"
Write-Host " 1.3 Generating Synthetic PDF Graphs"
$PythonCmd = ""
if (Get-Command py -ErrorAction SilentlyContinue) { $PythonCmd = "py" }
elseif (Get-Command python -ErrorAction SilentlyContinue) { 
    $test = & python -V 2>&1
    if ("$test" -match "Python") { $PythonCmd = "python" }
}
elseif (Get-Command python3 -ErrorAction SilentlyContinue) { $PythonCmd = "python3" }
if ($PythonCmd) {
    Write-Host "Plotting latency distributions..."
    & $PythonCmd scripts\visualization\plot_latency.py --input results\exp1_symmetric\summary.json --output "$PDF_DIR\exp1_latency.pdf"
    
    Write-Host "Plotting Pareto curves..."
    & $PythonCmd scripts\visualization\plot_pareto_curve.py --input results\exp2_asymmetric\summary.json --output "$PDF_DIR\exp2_pareto.pdf"
    
    Write-Host "Plotting Fairness..."
    & $PythonCmd scripts\visualization\plot_fairness.py --input results\exp3_high_contention\summary.json --output "$PDF_DIR\exp3_fairness.pdf"
} else {
    Write-Host "Warning: Python not found. Skipping plot generation." -ForegroundColor Yellow
}

Write-Host "`n======================================"
Write-Host " PHASE 2: TRACE DATA PROFILING & CLEANING"
Write-Host "======================================"
Write-Host " 2.1 Analyzing and Building Real-World Datasets"
.\prepare_traces.ps1

Write-Host "`n======================================"
Write-Host " PHASE 3: TRACE-DRIVEN BENCHMARKS using scheduler: $Scheduler"
Write-Host "======================================"
Write-Host " 3.1 Executing Simulator on Cleaned Traces Separately"
$TRACES = @("google", "azure", "alibaba")

foreach ($trace_name in $TRACES) {
    $TRACE_FILE = "$TRACE_OUTPUT_DIR\$trace_name\canonical.csv"
    $RESULT_DIR = "results\trace_eval_$trace_name"
    
    Write-Host "--------------------------------------"
    if (Test-Path $TRACE_FILE) {
        Write-Host "Running trace evaluation for: $($trace_name)"
        & $EMBI_SIM --scheduler $Scheduler --workload trace --trace "$TRACE_FILE" --output "$RESULT_DIR"
        Write-Host "Saved results to: $RESULT_DIR"
        
        if ($PythonCmd -and (Test-Path "$RESULT_DIR\run.csv")) {
            & $PythonCmd scripts\visualization\plot_trace_results.py --input "$RESULT_DIR\run.csv" --output "$PDF_DIR\${trace_name}_sim_results.pdf"
        }
    } else {
        Write-Host "Skipping $($trace_name): Canonical trace file not found at $TRACE_FILE." -ForegroundColor Yellow
    }
}

Write-Host "======================================"
Write-Host " Pipeline Complete."
Write-Host " Synthetic results available in results\exp*\"
Write-Host " Trace results available in results\trace_eval_*\"
Write-Host " Graphs available in $PDF_DIR\"
Write-Host "======================================"
