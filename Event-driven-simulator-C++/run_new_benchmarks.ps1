
$ErrorActionPreference = "Stop"

Write-Host "`n========== Phase 2: Supplemental Pipeline (RQ6 & LowLoad) =========="

$rq_configs = @(
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

Write-Host "`nSupplemental Pipeline Execution Complete!"

