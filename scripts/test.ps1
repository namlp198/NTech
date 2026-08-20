[CmdletBinding()]
param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Release",

    [switch]$Build
)

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot
$localDotnet = Join-Path $repoRoot "artifacts/dotnet/dotnet.exe"
$dotnet = if (Test-Path -LiteralPath $localDotnet) { $localDotnet } else { "dotnet" }

Push-Location $repoRoot
try {
    & "$PSScriptRoot\native-test.ps1" -Configuration $Configuration
    if ($LASTEXITCODE -ne 0) {
        throw "Native tests failed with exit code $LASTEXITCODE"
    }

    $arguments = @("test", "NTech.NVision.sln", "--configuration", $Configuration, "--no-restore")
    if (-not $Build) {
        $arguments += "--no-build"
    }

    & $dotnet @arguments
    if ($LASTEXITCODE -ne 0) {
        throw "dotnet test failed with exit code $LASTEXITCODE"
    }
}
finally {
    Pop-Location
}
