#requires -Version 7.0
param([string]$OutputDirectory = (Join-Path ([IO.Path]::GetTempPath()) ('GeoViz-link-checks-' + [guid]::NewGuid())))
$ErrorActionPreference = 'Stop'
$fixture = [IO.Path]::GetFullPath($OutputDirectory)
if (Test-Path -LiteralPath $fixture) { throw 'Link regression output must be a new directory.' }
$docs = Join-Path $fixture 'docs'
New-Item -ItemType Directory -Path $docs | Out-Null
Set-Content -LiteralPath (Join-Path $docs 'guide.md') -Value '# Guide'
Set-Content -LiteralPath (Join-Path $docs 'guide (draft).md') -Value '# Draft'
Set-Content -LiteralPath (Join-Path $docs 'plot & legend.png') -Value 'Image path fixture'
$cases = @(
    @{
        Name = 'fenced and inline C++ lambdas are not links'
        Markdown = @'
```cpp
auto add = [this](std::unique_ptr<Algorithm> algorithm) {};
[example](missing-code-only.md)
```
An inline lambda is `[this](missing-inline.md)`.
[Guide](docs/guide.md)
'@
    },
    @{
        Name = 'long and tilde fences preserve code examples'
        Markdown = @'
````cpp
```
[example](missing-long-fence.md)
````
~~~cpp
[example](missing-tilde-fence.md)
~~~
[Guide](docs/guide.md)
'@
    },
    @{
        Name = 'reference links encoded paths images and external URLs'
        Markdown = @'
[Guide][local]
[Draft](<docs/guide (draft).md>)
![Plot](docs/plot%20%26%20legend.png)
[External](https://example.com/missing.md)
[Contact](mailto:example@example.com)
[Section](#heading)

[local]: docs/guide.md#heading
'@
    },
    @{
        Name = 'a real missing link fails'
        Markdown = '[Missing](docs/missing-guide.md)'
        Missing = 'docs/missing-guide.md'
    },
    @{
        Name = 'a real missing image fails'
        Markdown = '![Missing](docs/missing-image.png)'
        Missing = 'docs/missing-image.png'
    },
    @{
        Name = 'a missing reference link fails'
        Markdown = "[Missing][ref]`n`n[ref]: docs/missing-reference.md"
        Missing = 'docs/missing-reference.md'
    },
    @{
        Name = 'a missing link after a code fence still fails'
        Markdown = @'
```cpp
auto add = [this](std::unique_ptr<Algorithm> algorithm) {};
```
[Missing](docs/missing-after-code.md)
'@
        Missing = 'docs/missing-after-code.md'
    }
)
foreach ($case in $cases) {
    Set-Content -LiteralPath (Join-Path $fixture 'README.md') -Value $case.Markdown
    $failure = $null
    try { & (Join-Path $PSScriptRoot 'documentation_links.ps1') -Root $fixture | Out-Null }
    catch { $failure = $_.Exception.Message }
    if ($case.Missing) {
        if (-not $failure -or -not $failure.Contains($case.Missing)) {
            throw "Regression failed: $($case.Name); expected the missing target, received: $failure"
        }
    } elseif ($failure) {
        throw "Regression failed: $($case.Name); $failure"
    }
}
Write-Output "PASS: $($cases.Count) documentation link regression cases."
