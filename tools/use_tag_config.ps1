$repoRoot = Split-Path -Parent $PSScriptRoot
Copy-Item -Path (Join-Path $repoRoot 'sdkconfig.tag') -Destination (Join-Path $repoRoot 'sdkconfig') -Force
Write-Host 'Applied sdkconfig.tag -> sdkconfig'
