param(
    [switch]$Run, [switch]$Smoke, [switch]$DebugBuild, [switch]$Docs,
    [string]$CompilerPath = 'C:\raylib\w64devkit\bin\g++.exe',
    [string]$RaylibRoot = 'C:\raylib\w64devkit'
)
$ErrorActionPreference = 'Stop'
$projectRoot = $PSScriptRoot
$env:PATH = (Split-Path -Parent $CompilerPath) + ';' + $env:PATH
$kind = if ($DebugBuild) { 'Debug' } else { 'Release' }
$buildDir = Join-Path $projectRoot ('build\' + $kind)
function Invoke-Checked([scriptblock]$Action) { & $Action; if ($LASTEXITCODE -ne 0) { throw 'Command failed; see diagnostics above.' } }
Invoke-Checked { cmake -S $projectRoot -B $buildDir -G 'MinGW Makefiles' "-DCMAKE_CXX_COMPILER=$CompilerPath" "-DRAYLIB_ROOT=$RaylibRoot" "-DCMAKE_BUILD_TYPE=$kind" }
Invoke-Checked { cmake --build $buildDir --parallel 4 }
Invoke-Checked { ctest --test-dir $buildDir --output-on-failure }
Copy-Item -LiteralPath (Join-Path $buildDir 'geoviz.exe') -Destination (Join-Path $projectRoot 'GeoViz.exe') -Force
if ($Docs) {
    Push-Location $projectRoot
    try {
        Invoke-Checked { & (Join-Path $buildDir 'geoviz_samples.exe') $projectRoot }
        Invoke-Checked { & (Join-Path $buildDir 'geoviz_showcase.exe') $projectRoot }
        Invoke-Checked { python tools/build_catalogue.py }
        Invoke-Checked { python tools/build_media.py }
        Invoke-Checked { npm ci }
        Invoke-Checked { node tools/build_docs.mjs }
        Invoke-Checked { node tools/verify_docs.mjs }
        Invoke-Checked { python tools/build_report.py }
    } finally { Pop-Location }
}
if ($Smoke) { Invoke-Checked { & (Join-Path $buildDir 'geoviz.exe') --root $projectRoot --smoke } }
elseif ($Run) { Invoke-Checked { cmake --build $buildDir --target run } }
