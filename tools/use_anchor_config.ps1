$repoRoot = Split-Path -Parent $PSScriptRoot
Copy-Item -Path (Join-Path $repoRoot 'sdkconfig.anchor') -Destination (Join-Path $repoRoot 'sdkconfig') -Force
Write-Host 'Applied sdkconfig.anchor -> sdkconfig'
