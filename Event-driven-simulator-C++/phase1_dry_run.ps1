$ErrorActionPreference = "Stop"

$BIN = ".\build_release_new\bin\embi_sim.exe"
$OUT_DIR = "results/raw_dryrun_2"

if (Test-Path $OUT_DIR) { Remove-Item $OUT_DIR -Recurse -Force }
New-Item -ItemType Directory -Force -Path $OUT_DIR | Out-Null

$RATES = @(0.5)
$ALL_SCHEDULERS = @("embi", "embi_oracle", "embi_unclipped", "maxweight", "rr", "fcfs", "cmu")
$WORKLOADS = @("uniform", "poisson", "bursty", "heavy_tail")

foreach ($wl in $WORKLOADS) {
    foreach ($sched in $ALL_SCHEDULERS) {
        $dir = "$OUT_DIR/Phase1/$wl/rate_0.5/$sched/seed_1"
        New-Item -ItemType Directory -Force -Path $dir | Out-Null
        $args_str = "--scheduler $sched --workload $wl --arrival-rate 0.5 --seed 1 --ticks 100000 --log-freq 100000 --output $dir"
        Invoke-Expression "$BIN $args_str"
    }
}
Write-Host "Dry run complete!"
