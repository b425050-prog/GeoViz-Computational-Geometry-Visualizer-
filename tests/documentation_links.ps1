#requires -Version 7.0
param([Parameter(Mandatory = $true)][string]$Root)
$ErrorActionPreference = 'Stop'
$rootPath = (Resolve-Path -LiteralPath $Root).Path
$documents = @(Get-Item -LiteralPath (Join-Path $rootPath 'README.md')) +
    @(Get-ChildItem -LiteralPath (Join-Path $rootPath 'docs') -Filter '*.md' -File)
$checked = 0
foreach ($document in $documents) {
    # Render Markdown before finding links so fenced and inline code remain literal text.
    $html = (ConvertFrom-Markdown -InputObject ([IO.File]::ReadAllText($document.FullName))).Html
    foreach ($link in [regex]::Matches($html, '(?i)<(?:a|img)\b[^>]*?\b(?:href|src)="(?<target>[^"]+)"')) {
        $target = [Net.WebUtility]::HtmlDecode($link.Groups['target'].Value)
        if ($target -match '^(?:[a-z][a-z0-9+.-]*:|//|#)') { continue }
        $relative = [Uri]::UnescapeDataString(($target -split '[#?]', 2)[0])
        $path = Join-Path $document.DirectoryName $relative
        if (-not (Test-Path -LiteralPath $path)) {
            throw "Broken packaged documentation link in $($document.Name): $target"
        }
        ++$checked
    }
}
Write-Output "PASS: $checked local documentation links across $($documents.Count) Markdown files."
