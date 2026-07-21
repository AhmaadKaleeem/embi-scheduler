$ErrorActionPreference = "Stop"

$Ms = @(2, 4, 6, 8, 10, 15, 20)
$Results = @()

foreach ($m in $Ms) {
    Write-Host "Running M=$m..."
    $outDir = "results/sensitivity_m_$m"
    .\build_release\bin\embi_sim.exe --ticks 1000000 --scheduler embi --arrival-rate 0.05 --M $m --log-freq 1 --output $outDir
    
    # Calculate diagnostics
    $jsonOut = python scripts/expanded_diagnostics.py "$outDir/run.csv" | ConvertFrom-Json
    
    $obj = [PSCustomObject]@{
        M = $m
        P99 = $jsonOut.'Queue Statistics'.'99th'
        ContextSwitches = $jsonOut.'Context Switches / 1M Ticks'
        MeanQueue = $jsonOut.'Queue Statistics'.'mean'
    }
    $Results += $obj
}

Write-Host "`n--- M Sensitivity Results ---"
$Results | Format-Table
$Results | Export-Csv "results/m_sensitivity_summary.csv" -NoTypeInformation
