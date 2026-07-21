$ErrorActionPreference = "Stop"

$NoiseLevels = @(0.0, 0.01, 0.05, 0.1, 0.2, 0.5, 1.0)
$Results = @()

foreach ($n in $NoiseLevels) {
    Write-Host "Running Hybrid Noise=$n..."
    $outDir = "results/hybrid_noise_$n"
    
    # Run simulation (1M ticks for accuracy)
    .\build_release\bin\embi_sim.exe --ticks 1000000 --scheduler hybrid_embi --arrival-rate-asymmetric 0.1,0.9,0.1,0.9,0.1,0.9,0.1,0.9,0.1,0.9,0.1,0.9,0.1,0.9,0.1,0.9 --lambda-noise $n --output $outDir --log-freq 1
    
    # Calculate diagnostics
    $jsonOut = python scripts/expanded_diagnostics.py "$outDir/run.csv" | ConvertFrom-Json
    
    $obj = [PSCustomObject]@{
        NoiseLevel = $n
        FallbackPct = $jsonOut.'Hybrid Statistics'.fallback_pct
        EMBIPct = $jsonOut.'Hybrid Statistics'.embi_pct
        MeanGap = $jsonOut.'Hybrid Statistics'.mean_gap
        MeanTau = $jsonOut.'Hybrid Statistics'.mean_tau
        TransitionCount = $jsonOut.'Hybrid Statistics'.transition_count
        AvgEMBIStreak = $jsonOut.'Hybrid Statistics'.avg_embi_streak
    }
    $Results += $obj
}

Write-Host "`n--- Hybrid Noise Sweep Results ---"
$Results | Format-Table
$Results | Export-Csv "results/hybrid_noise_summary.csv" -NoTypeInformation
