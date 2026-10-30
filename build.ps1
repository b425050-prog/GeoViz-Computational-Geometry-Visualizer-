param(
    [switch]$Run, [switch]$Smoke, [switch]$DebugBuild, [switch]$CoreOnly,
    [string]$CompilerPath = 'C:\raylib\w64devkit\bin\g++.exe',
    [string]$RaylibRoot = 'C:\raylib\w64devkit'
)
$ErrorActionPreference = 'Stop'
if ($CoreOnly -and ($Run -or $Smoke)) { throw 'Run and Smoke require the desktop application.' }
if (-not (Test-Path -LiteralPath $CompilerPath)) { throw "Compiler not found: $CompilerPath" }
$env:PATH = (Split-Path -Parent $CompilerPath) + ';' + $env:PATH
$kind = if ($DebugBuild) { 'Debug' } else { 'Release' }
$folder = if ($CoreOnly) { 'Core-' + $kind } else { $kind }
$buildDir = Join-Path $PSScriptRoot ('build\' + $folder)
function Invoke-Checked([scriptblock]$Action) {
    & $Action
    if ($LASTEXITCODE -ne 0) { throw 'Command failed; see diagnostics above.' }
}
$app = if ($CoreOnly) { 'OFF' } else { 'ON' }
Invoke-Checked { cmake -S $PSScriptRoot -B $buildDir -G Ninja "-DCMAKE_CXX_COMPILER=$CompilerPath" "-DRAYLIB_ROOT=$RaylibRoot" "-DCMAKE_BUILD_TYPE=$kind" "-DGEOVIZ_BUILD_APP=$app" }
Invoke-Checked { cmake --build $buildDir --parallel 4 }
Invoke-Checked { ctest --test-dir $buildDir --output-on-failure }
if (-not $CoreOnly) {
    $executable = Join-Path $PSScriptRoot 'GeoViz.exe'
    Copy-Item -LiteralPath (Join-Path $buildDir 'geoviz.exe') -Destination $executable -Force
    if ($Smoke) { Invoke-Checked { & $executable --root $PSScriptRoot --smoke } }
    elseif ($Run) { Invoke-Checked { & $executable --root $PSScriptRoot } }
}
