$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$buildDir = Join-Path $repoRoot 'testing\boot_swap_esp32'
$projectDir = Join-Path $repoRoot 'OTA\flash_lab'

Push-Location $projectDir
try {
    idf.py -B $buildDir -D "SDKCONFIG=$buildDir\sdkconfig" build
    if ($LASTEXITCODE -ne 0) { throw 'Boot swap flash lab build failed' }
} finally {
    Pop-Location
}
