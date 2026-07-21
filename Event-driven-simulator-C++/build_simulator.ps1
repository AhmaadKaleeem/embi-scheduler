Write-Host "======================================"
Write-Host " COMPILE & BUILD SIMULATOR (2-in-1)"
Write-Host "======================================"

$CMakeGenerator = ""
if (Get-Command gcc -ErrorAction SilentlyContinue) {
    $CMakeGenerator = '-G "MinGW Makefiles"'
    Write-Host "Detected MinGW. Using generator: $CMakeGenerator" -ForegroundColor Green
} else {
    Write-Host "MinGW not found. Falling back to default MSVC/NMake." -ForegroundColor Yellow
}

Write-Host "`n[1/2] Configuring CMake..." -ForegroundColor Cyan
Invoke-Expression "cmake $CMakeGenerator -B build_release -DCMAKE_BUILD_TYPE=Release"

if ($LASTEXITCODE -ne 0) {
    Write-Host "CMake configuration failed!" -ForegroundColor Red
    exit $LASTEXITCODE
}

Write-Host "`n[2/2] Compiling Simulator..." -ForegroundColor Cyan
cmake --build build_release --config Release

if ($LASTEXITCODE -eq 0) {
    Write-Host "`nBuild Succeeded!" -ForegroundColor Green
    
    $EMBI_SIM = ""
    if (Test-Path ".\build_release\Release\embi_sim.exe") {
        $EMBI_SIM = ".\build_release\Release\embi_sim.exe"
    } elseif (Test-Path ".\build_release\bin\embi_sim.exe") {
        $EMBI_SIM = ".\build_release\bin\embi_sim.exe"
    } elseif (Test-Path ".\build_release\embi_sim.exe") {
        $EMBI_SIM = ".\build_release\embi_sim.exe"
    }
    
    if ($EMBI_SIM) {
        Write-Host "Executable generated at: $EMBI_SIM" -ForegroundColor Cyan
    } else {
        Write-Host "Warning: Could not locate embi_sim.exe!" -ForegroundColor Yellow
    }
} else {
    Write-Host "`nBuild Failed!" -ForegroundColor Red
    exit $LASTEXITCODE
}
