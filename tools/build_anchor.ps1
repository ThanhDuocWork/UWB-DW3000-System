$repoRoot = Split-Path -Parent $PSScriptRoot
$sdkconfig = Join-Path $repoRoot "build_anchor\sdkconfig"
$defaults = "$(Join-Path $repoRoot 'sdkconfig.defaults');$(Join-Path $repoRoot 'sdkconfig.anchor')"

Push-Location $repoRoot
try {
    idf.py -B build_anchor -D "SDKCONFIG=$sdkconfig" -D "SDKCONFIG_DEFAULTS=$defaults" build
} finally {
    Pop-Location
}
