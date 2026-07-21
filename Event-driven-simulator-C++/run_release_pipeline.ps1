# run_release_pipeline.ps1
# Canonical benchmark suite for USENIX ATC submission.
# Generates results with complete provenance (hashes, versions, full config parameters)

$ErrorActionPreference = "Stop"

# ── 1. Provenance Hashing ──
Write-Host "========== Phase A: Provenance Collection =========="
$gitCommit = "unknown"
if (Get-Command git -ErrorAction SilentlyContinue) {
    $gitCommit = (git rev-parse HEAD)
}
Write-Host "Git Commit: $gitCommit"

$binaryPath = "build_release\bin\embi_sim.exe"
if (-not (Test-Path $binaryPath)) {
    Write-Error "Simulator binary not found. Please compile the simulator first."
}
$binaryHash = (Get-FileHash $binaryPath -Algorithm SHA256).Hash
Write-Host "Binary SHA256: $binaryHash"

$configHash = "none" # Not using a YAML config file for these runs, command line overrides

# ── 2. Clean Environment ──
Write-Host "`n========== Phase B: Environment Prep =========="
if (Test-Path "results") {
    Remove-Item -Recurse -Force "results"
}
New-Item -ItemType Directory -Force -Path "results/raw" | Out-Null
New-Item -ItemType Directory -Force -Path "results/aggregated" | Out-Null

# ── 3. Definition of Experiments ──
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
        id = "RQ7_LowLoad"
        desc = "Focus on P99 latency reduction vs MaxWeight under unsaturated queue conditions."
        schedulers = @("fcfs", "rr", "maxweight", "embi")
        workloads = @("poisson", "bursty")
        extra_flags = "--arrival-rate 0.05"
    },
    @{ id = "Sensitivity_M_2"; desc = "M=2"; schedulers = @("embi"); workloads = @("poisson"); extra_flags = "--arrival-rate 0.5 --M 2" },
    @{ id = "Sensitivity_M_4"; desc = "M=4"; schedulers = @("embi"); workloads = @("poisson"); extra_flags = "--arrival-rate 0.5 --M 4" },
    @{ id = "Sensitivity_M_6"; desc = "M=6"; schedulers = @("embi"); workloads = @("poisson"); extra_flags = "--arrival-rate 0.5 --M 6" },
    @{ id = "Sensitivity_M_8"; desc = "M=8"; schedulers = @("embi"); workloads = @("poisson"); extra_flags = "--arrival-rate 0.5 --M 8" },
    @{ id = "Sensitivity_M_10"; desc = "M=10"; schedulers = @("embi"); workloads = @("poisson"); extra_flags = "--arrival-rate 0.5 --M 10" },
    @{ id = "Sensitivity_M_15"; desc = "M=15"; schedulers = @("embi"); workloads = @("poisson"); extra_flags = "--arrival-rate 0.5 --M 15" },
    @{ id = "Sensitivity_M_20"; desc = "M=20"; schedulers = @("embi"); workloads = @("poisson"); extra_flags = "--arrival-rate 0.5 --M 20" },

    @{ id = "Sensitivity_Noise_0"; desc = "Noise=0"; schedulers = @("hybrid_embi"); workloads = @("poisson"); extra_flags = "--arrival-rate 0.5 --lambda-noise 0" },
    @{ id = "Sensitivity_Noise_0.01"; desc = "Noise=0.01"; schedulers = @("hybrid_embi"); workloads = @("poisson"); extra_flags = "--arrival-rate 0.5 --lambda-noise 0.01" },
    @{ id = "Sensitivity_Noise_0.05"; desc = "Noise=0.05"; schedulers = @("hybrid_embi"); workloads = @("poisson"); extra_flags = "--arrival-rate 0.5 --lambda-noise 0.05" },
    @{ id = "Sensitivity_Noise_0.1"; desc = "Noise=0.1"; schedulers = @("hybrid_embi"); workloads = @("poisson"); extra_flags = "--arrival-rate 0.5 --lambda-noise 0.1" },
    @{ id = "Sensitivity_Noise_0.2"; desc = "Noise=0.2"; schedulers = @("hybrid_embi"); workloads = @("poisson"); extra_flags = "--arrival-rate 0.5 --lambda-noise 0.2" },
    @{ id = "Sensitivity_Noise_0.5"; desc = "Noise=0.5"; schedulers = @("hybrid_embi"); workloads = @("poisson"); extra_flags = "--arrival-rate 0.5 --lambda-noise 0.5" }
)

Write-Host "`n========== Phase C: Execution =========="
foreach ($rq in $rq_configs) {
    Write-Host "`n--- Executing $($rq.id): $($rq.desc) ---"
    
    foreach ($workload in $rq.workloads) {
        foreach ($scheduler in $rq.schedulers) {
            
            # Using Seeds 1..50
            foreach ($seed in 1..50) {
                $outputDir = "results/raw/$($rq.id)/$workload/$scheduler/seed_$seed"
                
                $cmd = "./$binaryPath --scheduler $scheduler --output $outputDir --null-log --seed $seed"
                $cmd += " --git-commit $gitCommit --binary-hash $binaryHash --config-hash $configHash"
                
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

Write-Host "`n========== Phase D: Trace Verification Extraction =========="
# Appendix A: Extract the human readable trace for validation
$traceOutput = "results/appendix_a_human_trace.txt"
$cmd = "./$binaryPath --scheduler embi --arrival-rate 0.95 --M 10 --human-trace --ticks 250 --null-log > $traceOutput"
Invoke-Expression $cmd
if ($LASTEXITCODE -ne 0) {
    Write-Error "Trace generation failed."
} else {
    Write-Host "Trace generated at $traceOutput"
}

Write-Host "`nRelease Pipeline Execution Complete!"
Write-Host "Raw artifacts generated in results/raw/."
