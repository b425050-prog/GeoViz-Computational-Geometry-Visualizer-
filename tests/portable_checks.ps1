param(
    [Parameter(Mandatory = $true)][string]$BuildDirectory,
    [string]$OutputDirectory = (Join-Path ([IO.Path]::GetTempPath()) ('GeoViz-package-' + [guid]::NewGuid()))
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$buildRoot = (Resolve-Path -LiteralPath $BuildDirectory).Path
$artifactRoot = [IO.Path]::GetFullPath($OutputDirectory)
if (Test-Path -LiteralPath $artifactRoot) { throw 'Package test output must be a new directory.' }
New-Item -ItemType Directory -Path $artifactRoot | Out-Null
$payload = Join-Path $artifactRoot 'package\GeoViz'
cmake --install $buildRoot --prefix $payload
if ($LASTEXITCODE -ne 0) { throw 'Installing the fresh build failed.' }
$archive = Join-Path $artifactRoot 'GeoViz-Windows.zip'
Compress-Archive -LiteralPath $payload -DestinationPath $archive -CompressionLevel Optimal
$extraction = Join-Path $artifactRoot ('Fresh extraction ' + [char]0x03A9 + ' with spaces')
Expand-Archive -LiteralPath $archive -DestinationPath $extraction
$runtime = Join-Path $extraction 'GeoViz'
$exe = Join-Path $runtime 'geoviz.exe'
if ((Get-FileHash -LiteralPath $exe).Hash -ne (Get-FileHash -LiteralPath (Join-Path $buildRoot 'geoviz.exe')).Hash) {
    throw 'The packaged executable differs from the tested build.'
}
foreach ($asset in @('assets/Lato-Regular.ttf','assets/Lato-Bold.ttf','assets/geoviz.ico','assets/geoviz.png')) {
    if ((Get-FileHash -LiteralPath (Join-Path $runtime $asset)).Hash -ne (Get-FileHash -LiteralPath (Join-Path $projectRoot $asset)).Hash) {
        throw "Packaged asset differs from source: $asset"
    }
}
function Invoke-Smoke([string]$Root, [int]$Width, [int]$Height, [int]$ExpectedExit = 0) {
    $logName = 'process-' + [guid]::NewGuid()
    $stdout = Join-Path $artifactRoot ($logName + '.stdout.txt')
    $stderr = Join-Path $artifactRoot ($logName + '.stderr.txt')
    $process = New-Object System.Diagnostics.Process
    $process.StartInfo.FileName = Join-Path $Root 'geoviz.exe'
    $process.StartInfo.Arguments = '--root "' + $Root + '" --smoke --smoke-size ' + $Width + ' ' + $Height
    $process.StartInfo.UseShellExecute = $false
    $process.StartInfo.CreateNoWindow = $true
    $process.StartInfo.WindowStyle = [Diagnostics.ProcessWindowStyle]::Hidden
    $process.StartInfo.RedirectStandardOutput = $true
    $process.StartInfo.RedirectStandardError = $true
    $process.StartInfo.StandardOutputEncoding = New-Object Text.UTF8Encoding($false)
    $process.StartInfo.StandardErrorEncoding = New-Object Text.UTF8Encoding($false)
    $null = $process.Start()
    $outTask = $process.StandardOutput.ReadToEndAsync()
    $errorTask = $process.StandardError.ReadToEndAsync()
    if (-not $process.WaitForExit(120000)) {
        $process.Kill()
        throw 'The packaged desktop smoke test timed out.'
    }
    $process.WaitForExit()
    [IO.File]::WriteAllText($stdout, $outTask.Result)
    [IO.File]::WriteAllText($stderr, $errorTask.Result)
    $message = $errorTask.Result
    if ($process.ExitCode -ne $ExpectedExit) { throw "Unexpected exit code $($process.ExitCode): $message" }
    return $message
}
foreach ($size in @(@(960,640),@(1440,900),@(1920,1200))) {
    Invoke-Smoke $runtime $size[0] $size[1] | Out-Null
    $report = Get-Content -Raw -LiteralPath (Join-Path $runtime 'output/APP_SMOKE_REPORT.txt')
    $features = Get-Content -Raw -LiteralPath (Join-Path $runtime 'output/FEATURE_CHECKS.txt')
    if ($report -notmatch 'rendered and captured 77 states' -or $features -notmatch '; 0 failures') {
        throw 'The packaged desktop checks did not complete.'
    }
    Copy-Item -LiteralPath (Join-Path $runtime 'output/APP_SMOKE_REPORT.txt') -Destination (Join-Path $artifactRoot ("desktop-$($size[0])x$($size[1]).txt"))
}
function Copy-Runtime([string]$Destination) {
    New-Item -ItemType Directory -Path $Destination | Out-Null
    Copy-Item -LiteralPath (Join-Path $runtime 'assets'),(Join-Path $runtime 'data'),$exe -Destination $Destination -Recurse
}
$missing = Join-Path $artifactRoot 'missing-font'
Copy-Runtime $missing
Rename-Item -LiteralPath (Join-Path $missing 'assets/Lato-Regular.ttf') -NewName 'Lato-Regular.ttf.disabled'
$message = Invoke-Smoke $missing 960 640 1
if ($message -notmatch 'Lato-Regular.ttf' -or $message -notmatch 'Extract the complete') { throw 'Missing-asset guidance is unclear.' }
$malformed = Join-Path $artifactRoot 'malformed-data'
Copy-Runtime $malformed
Set-Content -LiteralPath (Join-Path $malformed 'data/security_sample.csv') -Value 'invalid CSV'
if ((Invoke-Smoke $malformed 960 640 1) -notmatch 'CSV header') { throw 'Malformed CSV is not reported.' }
foreach ($document in @(Get-Item -LiteralPath (Join-Path $runtime 'README.md')) + @(Get-ChildItem -LiteralPath (Join-Path $runtime 'docs') -Filter '*.md' -File)) {
    foreach ($link in [regex]::Matches([IO.File]::ReadAllText($document.FullName), '\]\(([^)]+)\)')) {
        $target = $link.Groups[1].Value
        if ($target -match '^(https?://|#)') { continue }
        $path = Join-Path $document.DirectoryName ($target.Split('#')[0])
        if (-not (Test-Path -LiteralPath $path)) { throw "Broken packaged documentation link: $target" }
    }
}
$digest = (Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash.ToLowerInvariant()
Set-Content -LiteralPath ($archive + '.sha256') -Value ($digest + '  GeoViz-Windows.zip') -Encoding ascii
Set-Content -LiteralPath (Join-Path $artifactRoot 'PACKAGE_CHECKS.txt') -Value `
    'PASS: fresh build/package equality; original fonts and icons; Unicode extraction path; three render sizes; missing fonts; malformed CSV; documentation links.'
Write-Output "PASS: Windows portable package verified: $archive"
