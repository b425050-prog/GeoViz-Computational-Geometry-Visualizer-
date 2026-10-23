param(
    [switch]$Run, [switch]$Smoke, [switch]$DebugBuild, [switch]$Docs,
    [string]$CompilerPath = 'C:\raylib\w64devkit\bin\g++.exe',
    [string]$RaylibRoot = 'C:\raylib\w64devkit'
)
$ErrorActionPreference = 'Stop'
$projectRoot = $PSScriptRoot
if (-not (Test-Path -LiteralPath $CompilerPath)) { throw 'Compiler not found. Supply -CompilerPath and a matching -RaylibRoot.' }
New-Item -ItemType Directory -Force -Path (Join-Path $projectRoot 'build') | Out-Null
$commonArgs = @('-std=c++17','-Wall','-Wextra','-Wpedantic','-Werror','-static','-I', (Join-Path $projectRoot 'include'))
if ($DebugBuild) { $commonArgs += @('-O0','-g','-D_GLIBCXX_ASSERTIONS') } else { $commonArgs += '-O2' }
$engineSources = @((Join-Path $projectRoot 'src\geometry.cpp'),(Join-Path $projectRoot 'src\algorithms.cpp'),(Join-Path $projectRoot 'src\io.cpp'),(Join-Path $projectRoot 'src\samples.cpp'),(Join-Path $projectRoot 'src\security_samples.cpp'))
Write-Host 'Building engine tests...'
& $compilerPath @commonArgs @engineSources (Join-Path $projectRoot 'tests\tests.cpp') '-o' (Join-Path $projectRoot 'build\geoviz_tests.exe')
if ($LASTEXITCODE -ne 0) { throw 'Test build failed.' }
Write-Host 'Running checks with isolated test artifacts...'
& (Join-Path $projectRoot 'build\geoviz_tests.exe') $projectRoot (Join-Path $projectRoot 'build\test-output')
if ($LASTEXITCODE -ne 0) { throw 'Checks failed.' }
Write-Host 'Building raylib application...'
$resourceArgs = @()
$resourceCompiler = Join-Path (Split-Path -Parent $CompilerPath) 'windres.exe'
if (Test-Path -LiteralPath $resourceCompiler) {
    Push-Location $projectRoot
    try {
        & $resourceCompiler 'assets\geoviz.rc' '-O' 'coff' '-o' 'build\geoviz-res.o'
        if ($LASTEXITCODE -ne 0) { throw 'Executable resource build failed.' }
    } finally { Pop-Location }
    $resourceArgs = @((Join-Path $projectRoot 'build\geoviz-res.o'))
}
& $CompilerPath @commonArgs '-I' (Join-Path $RaylibRoot 'include') @engineSources (Join-Path $projectRoot 'src\main.cpp') @resourceArgs '-L' (Join-Path $RaylibRoot 'lib') '-lraylib' '-lopengl32' '-lgdi32' '-lwinmm' '-o' (Join-Path $projectRoot 'build\geoviz.exe')
if ($LASTEXITCODE -ne 0) { throw 'Application build failed.' }
Copy-Item -LiteralPath (Join-Path $projectRoot 'build\geoviz.exe') -Destination (Join-Path $projectRoot 'GeoViz.exe') -Force
if ($Docs) {
    & $CompilerPath @commonArgs @engineSources (Join-Path $projectRoot 'tools\export_samples.cpp') '-o' (Join-Path $projectRoot 'build\geoviz_samples.exe')
    if ($LASTEXITCODE -ne 0) { throw 'Sample exporter build failed.' }
    & (Join-Path $projectRoot 'build\geoviz_samples.exe') $projectRoot
    if ($LASTEXITCODE -ne 0) { throw 'Checked sample export failed.' }
    & $CompilerPath @commonArgs @engineSources (Join-Path $projectRoot 'tools\export_showcase.cpp') '-o' (Join-Path $projectRoot 'build\geoviz_showcase.exe')
    if ($LASTEXITCODE -ne 0) { throw 'Trace exporter build failed.' }
    & (Join-Path $projectRoot 'build\geoviz_showcase.exe') $projectRoot
    if ($LASTEXITCODE -ne 0) { throw 'Trace export failed.' }
    & python (Join-Path $projectRoot 'tools\build_catalogue.py')
    if ($LASTEXITCODE -ne 0) { throw 'Sample catalogue generation failed.' }
    & python (Join-Path $projectRoot 'tools\build_media.py')
    if ($LASTEXITCODE -ne 0) { throw 'Documentation media generation failed.' }
    & node (Join-Path $projectRoot 'tools\build_docs.mjs')
    if ($LASTEXITCODE -ne 0) { throw 'Documentation render failed.' }
}
Write-Host 'Ready: GeoViz.exe. Read README.md or open docs\index.html.'
if ($Smoke) { & (Join-Path $projectRoot 'build\geoviz.exe') '--root' $projectRoot '--smoke'; if ($LASTEXITCODE -ne 0) { throw 'Visual smoke run failed.' } }
elseif ($Run) { & (Join-Path $projectRoot 'build\geoviz.exe') '--root' $projectRoot }
