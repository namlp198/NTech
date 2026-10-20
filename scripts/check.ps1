[CmdletBinding()]
param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Release",

    [switch]$NoRestore
)

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot
$localDotnet = Join-Path $repoRoot "artifacts/dotnet/dotnet.exe"
$dotnet = if (Test-Path -LiteralPath $localDotnet) { $localDotnet } else { "dotnet" }

Push-Location $repoRoot
try {
    if (-not $NoRestore) {
        & "$PSScriptRoot\restore.ps1"
        if ($LASTEXITCODE -ne 0) {
            throw "Restore check failed with exit code $LASTEXITCODE"
        }
    }

    & "$PSScriptRoot\build.ps1" -Configuration $Configuration
    if ($LASTEXITCODE -ne 0) {
        throw "Build check failed with exit code $LASTEXITCODE"
    }

    & "$PSScriptRoot\test.ps1" -Configuration $Configuration
    if ($LASTEXITCODE -ne 0) {
        throw "Test check failed with exit code $LASTEXITCODE"
    }

    & $dotnet format "NTech.NVision.sln" --verify-no-changes --no-restore
    if ($LASTEXITCODE -ne 0) {
        throw "Format check failed with exit code $LASTEXITCODE"
    }
}
finally {
    Pop-Location
}
