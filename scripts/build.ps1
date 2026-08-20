[CmdletBinding()]
param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Release",

    [switch]$Restore
)

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot
$localDotnet = Join-Path $repoRoot "artifacts/dotnet/dotnet.exe"
$dotnet = if (Test-Path -LiteralPath $localDotnet) { $localDotnet } else { "dotnet" }

Push-Location $repoRoot
try {
    & "$PSScriptRoot\native-build.ps1" -Configuration $Configuration
    if ($LASTEXITCODE -ne 0) {
        throw "Native build failed with exit code $LASTEXITCODE"
    }

    $arguments = @("build", "NTech.NVision.sln", "--configuration", $Configuration)
    if (-not $Restore) {
        $arguments += "--no-restore"
    }

    & $dotnet @arguments
    if ($LASTEXITCODE -ne 0) {
        throw "dotnet build failed with exit code $LASTEXITCODE"
    }
}
finally {
    Pop-Location
}
