# run_golden_pipeline.ps1
# Exhaustive benchmark sweep mapping directly to the Research Questions (RQ1 - RQ6)

param (
    [switch]$SkipSimulatorBuild,
    [switch]$SkipTraceParsing
)

$ErrorActionPreference = "Stop"

Write-Host "========== Phase 0: Regression Verification =========="
./run_verification.ps1

Write-Host "`n========== Phase 2: Golden Pipeline (RQ1 - RQ6) =========="
Write-Host "Cleaning results directory to guarantee reproducibility..."
if (Test-Path "results") {
    Remove-Item -Recurse -Force "results"
}
New-Item -ItemType Directory -Force -Path "results/raw" | Out-Null
New-Item -ItemType Directory -Force -Path "results/aggregated" | Out-Null
New-Item -ItemType Directory -Force -Path "results/paper/plots" | Out-Null
New-Item -ItemType Directory -Force -Path "results/paper/tables" | Out-Null

$rq_configs = @(
    @{
        id = "RQ1_Stability"
        desc = "Baseline execution validating negative Lyapunov drift."
        schedulers = @("maxweight", "embi", "embi_unclipped")
        workloads = @("uniform")
        extra_flags = ""
    },
    @{
        id = "RQ2_Latency"
        desc = "Focus on P99 latency reduction vs MaxWeight."
        schedulers = @("fcfs", "rr", "maxweight", "embi")
        workloads = @("poisson", "bursty")
        extra_flags = ""
    },
    @{
        id = "RQ3_Asymmetry_Scaling"
        desc = "Parameter sweep of arrival skew."
        schedulers = @("maxweight", "embi")
        workloads = @("uniform")
        extra_flags = "--arrival-rate-asymmetric 0.9,0.1" 
    },
    @{
        id = "RQ4_Symmetric_Failure"
        desc = "Baseline execution proving EMBI degrades under symmetry."
        schedulers = @("maxweight", "embi")
        workloads = @("uniform")
        extra_flags = "--arrival-rate-asymmetric 0.5,0.5"
    },
    @{
        id = "RQ5_Hybrid_Resolution"
        desc = "Executing Hybrid EMBI on the RQ4 symmetric workloads."
        schedulers = @("maxweight", "embi", "hybrid_embi")
        workloads = @("uniform")
        extra_flags = "--arrival-rate-asymmetric 0.5,0.5"
    },
    @{
        id = "RQ6_Cloud_Generalization"
        desc = "Executing the canonical Alibaba, Azure, and Google traces."
        schedulers = @("fcfs", "rr", "maxweight", "embi", "hybrid_embi")
        workloads = @("alibaba", "google", "azure")
        extra_flags = ""
    },
    @{
        id = "Sensitivity_Epsilon_01"
        desc = "epsilon_total = 0.01"
        schedulers = @("hybrid_embi")
        workloads = @("uniform")
        extra_flags = "--epsilon-total 0.01"
    },
    @{
        id = "Sensitivity_Epsilon_05"
        desc = "epsilon_total = 0.05"
        schedulers = @("hybrid_embi")
        workloads = @("uniform")
        extra_flags = "--epsilon-total 0.05"
    },
    @{
        id = "Sensitivity_Epsilon_20"
        desc = "epsilon_total = 0.20"
        schedulers = @("hybrid_embi")
        workloads = @("uniform")
        extra_flags = "--epsilon-total 0.20"
    },
    @{
        id = "Sensitivity_Noise"
        desc = "Noise Robustness."
        schedulers = @("embi")
        workloads = @("uniform")
        extra_flags = "--lambda-noise 0.1"
    }
)

foreach ($rq in $rq_configs) {
    Write-Host "`n--- Executing $($rq.id): $($rq.desc) ---"
    
    foreach ($workload in $rq.workloads) {
        foreach ($scheduler in $rq.schedulers) {
            # 50 seeds per configuration for statistical rigorousness
            foreach ($seed in 1..50) {
                $outputDir = "results/raw/$($rq.id)/$workload/$scheduler/seed_$seed"
                
                $cmd = "./build_release/bin/embi_sim.exe --scheduler $scheduler --output $outputDir --null-log --seed $seed"
                
                if ($workload -in @("alibaba", "google", "azure")) {
                    $traceFile = "scripts/trace_profiler/output_reports/$workload/canonical.csv"
                    if (-not (Test-Path $traceFile)) {
                        Write-Warning "Skipping $workload trace for $scheduler (trace file not found: $traceFile)"
                        continue
                    }
                    $cmd += " --workload trace --trace $traceFile"
                } else {
                    $cmd += " --workload $workload"
                }
                
                if ($rq.extra_flags) {
                    $cmd += " $($rq.extra_flags)"
                }
    
                Invoke-Expression $cmd
                if ($LASTEXITCODE -ne 0) {
                    Write-Error "Simulation failed for $scheduler on $workload seed $seed"
                    exit 1
                }
            }
            Write-Host "Completed 50 seeds for $scheduler on $workload"
        }
    }
}

Write-Host "`nGolden Pipeline Execution Complete!"
Write-Host "Raw artifacts generated in results/raw/."
Write-Host "Next step: Run Phase 3 Python Analytics (sanity_checks.py + generate_figures.py)"
