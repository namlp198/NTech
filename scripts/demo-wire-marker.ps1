[CmdletBinding()]
param(
    [string]$OutputRoot = "outputs/temporary/wire-marker-demo"
)

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot
$outputPath = [System.IO.Path]::GetFullPath((Join-Path $repoRoot $OutputRoot))

Push-Location $repoRoot
try {
    & dotnet run --project "apps/nvision-cli/NTech.NVision.Cli/NTech.NVision.Cli.csproj" `
        --configuration Release -- `
        inspect "samples/wire-marker/fixtures/type01.pgm" TYPE_01 (Join-Path $outputPath "accepted")
    if ($LASTEXITCODE -ne 0) {
        throw "TYPE_01 accepted demo failed with exit code $LASTEXITCODE"
    }

    & dotnet run --project "apps/nvision-cli/NTech.NVision.Cli/NTech.NVision.Cli.csproj" `
        --configuration Release -- `
        inspect "samples/wire-marker/fixtures/type04.pgm" TYPE_01 (Join-Path $outputPath "wrong-type")
    if ($LASTEXITCODE -ne 0) {
        throw "TYPE_04 wrong-type demo failed with exit code $LASTEXITCODE"
    }
}
finally {
    Pop-Location
}
