$repoRoot = Split-Path -Parent $PSScriptRoot
$sdkconfig = Join-Path $repoRoot "build_tag\sdkconfig"
$defaults = "$(Join-Path $repoRoot 'sdkconfig.defaults');$(Join-Path $repoRoot 'sdkconfig.tag')"

Push-Location $repoRoot
try {
    idf.py -B build_tag -D "SDKCONFIG=$sdkconfig" -D "SDKCONFIG_DEFAULTS=$defaults" build
} finally {
    Pop-Location
}
