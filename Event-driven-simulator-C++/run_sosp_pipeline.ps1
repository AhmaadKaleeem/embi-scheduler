$ErrorActionPreference = "Stop"

Write-Host "==========================================================="
Write-Host "   EMBI SOSP/OSDI Full Validation Suite"
Write-Host "==========================================================="

$BIN = ".\build_release_new\bin\embi_sim.exe"
$OUT_DIR = "results/raw"

# Clean previous runs
if (Test-Path $OUT_DIR) {
    Remove-Item $OUT_DIR -Recurse -Force
}
New-Item -ItemType Directory -Force -Path $OUT_DIR | Out-Null

$SEEDS = 1..50
$RATES = @(0.1, 0.3, 0.5, 0.7, 0.8, 0.9, 0.95, 0.98, 0.99)
$ALL_SCHEDULERS = @("embi", "hybrid_embi", "embi_oracle", "maxweight", "rr", "fcfs", "cmu")
$WORKLOADS = @("uniform", "poisson", "bursty", "heavy_tail")

function Run-Sim {
    param([string]$phase, [string]$wl, [string]$rate, [string]$sched, [string]$seed, [string]$extra_args)
    
    $dir = "$OUT_DIR/$phase/$wl/rate_$rate/$sched/seed_$seed"
    New-Item -ItemType Directory -Force -Path $dir | Out-Null
    
    $args_str = "--scheduler $sched --workload $wl --arrival-rate $rate --seed $seed --ticks 1000000 --log-freq 1000000 --output $dir $extra_args"
    Write-Host "Running: $phase | $wl | $sched | rate=$rate | seed=$seed"
    Invoke-Expression "$BIN $args_str"
}

# 1. RQ2_Latency & RQ4_Symmetric (Standard Load Sweep)
Write-Host "`n[Running RQ2_Latency & RQ4_Symmetric]"
foreach ($wl in $WORKLOADS) {
    foreach ($rate in $RATES) {
        foreach ($sched in $ALL_SCHEDULERS) {
            foreach ($seed in $SEEDS) {
                Run-Sim -phase "RQ2_Latency" -wl $wl -rate $rate -sched $sched -seed $seed -extra_args ""
            }
        }
    }
}

# 2. RQ7_LowLoad
Write-Host "`n[Running RQ7_LowLoad]"
foreach ($wl in $WORKLOADS) {
    foreach ($rate in @(0.01, 0.05, 0.1, 0.15)) {
        foreach ($sched in @("embi", "maxweight", "rr", "fcfs")) {
            foreach ($seed in $SEEDS) {
                Run-Sim -phase "RQ7_LowLoad" -wl $wl -rate $rate -sched $sched -seed $seed -extra_args ""
            }
        }
    }
}

# 3. RQ3_Asymmetry
Write-Host "`n[Running RQ3_Asymmetry]"
foreach ($sched in $ALL_SCHEDULERS) {
    foreach ($seed in $SEEDS) {
        $rate = "0.8"
        # Half processes at 0.1, half at 1.5
        $asym = "0.1,1.5"
        $extra = "--arrival-rate-asymmetric $asym"
        Run-Sim -phase "RQ3_Asymmetry" -wl "poisson" -rate $rate -sched $sched -seed $seed -extra_args $extra
    }
}

# 4. Sensitivity_M (RQ1_Stability & RQ5_Hybrid)
Write-Host "`n[Running Sensitivity_M]"
$M_VALS = @(0.0, 1.0, 5.0, 10.0, 20.0, 50.0)
foreach ($m in $M_VALS) {
    foreach ($seed in $SEEDS) {
        $rate = "0.95"
        $extra = "--M $m"
        Run-Sim -phase "Sensitivity_M" -wl "poisson" -rate $rate -sched "embi" -seed $seed -extra_args $extra
        Run-Sim -phase "Sensitivity_M" -wl "poisson" -rate $rate -sched "embi_oracle" -seed $seed -extra_args $extra
        Run-Sim -phase "Sensitivity_M" -wl "poisson" -rate $rate -sched "embi_unclipped" -seed $seed -extra_args $extra
    }
}

# 5. Sensitivity_Noise
Write-Host "`n[Running Sensitivity_Noise]"
$NOISE_VALS = @(0.1, 0.5, 1.0, 5.0, 10.0)
foreach ($noise in $NOISE_VALS) {
    foreach ($seed in $SEEDS) {
        $rate = "0.95"
        $extra = "--lambda-noise $noise"
        Run-Sim -phase "Sensitivity_Noise" -wl "poisson" -rate $rate -sched "embi" -seed $seed -extra_args $extra
    }
}

# 6. Trace
Write-Host "`n[Running Trace Workloads]"
foreach ($sched in $ALL_SCHEDULERS) {
    foreach ($seed in $SEEDS) {
        $dir = "$OUT_DIR/Trace/human_trace/rate_1.0/$sched/seed_$seed"
        New-Item -ItemType Directory -Force -Path $dir | Out-Null
        $extra = "--trace appendix_a_human_trace.txt"
        $args_str = "--scheduler $sched --workload trace --seed $seed --ticks 1000000 --log-freq 1000000 --output $dir $extra"
        Write-Host "Running: Trace | human_trace | $sched | seed=$seed"
        Invoke-Expression "$BIN $args_str"
    }
}

Write-Host "`nPipeline completed successfully!"
