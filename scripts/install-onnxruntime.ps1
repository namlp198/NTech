<#
.SYNOPSIS
Stages the pinned ONNX Runtime CPU distribution into artifacts/onnxruntime.

.DESCRIPTION
The native plane needs the ONNX Runtime C++ headers and import library, which the managed
NuGet reference does not put anywhere the C++ build can see: only the runtime binaries are
copied into bin/, and the headers stay in the package.

This stages the same package the managed plane already depends on, so both planes run one
ONNX Runtime version. That matters while the managed P2 path is still the behavior oracle
for native recognition work: a difference in results should mean a difference in our code,
not in the runtime underneath it.

The staged layout matches the official release archives, so ONNXRUNTIME_ROOT may point at
an official archive instead and cmake/FindONNXRuntime.cmake will resolve either.

Nothing is downloaded here. If the package is not in the NuGet cache the script says which
command fetches it and stops, rather than reaching out to the network on its own.
#>
[CmdletBinding()]
param(
    # Defaults to the version pinned for the managed plane in Directory.Packages.props.
    [string] $Version,
    [ValidateSet("win-x64", "win-arm64", "linux-x64", "linux-arm64", "osx-arm64")]
    [string] $Runtime = "win-x64",
    [switch] $Force
)

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot

if (-not $Version) {
    $packagesFile = Join-Path $repoRoot "Directory.Packages.props"
    $pinned = Select-String -Path $packagesFile -Pattern 'Microsoft\.ML\.OnnxRuntime"\s+Version="([^"]+)"'
    if (-not $pinned) {
        throw "Microsoft.ML.OnnxRuntime is not pinned in Directory.Packages.props; pass -Version explicitly."
    }
    $Version = $pinned.Matches[0].Groups[1].Value
}

$package = Join-Path $env:USERPROFILE ".nuget/packages/microsoft.ml.onnxruntime/$Version"
if (-not (Test-Path -LiteralPath $package)) {
    Write-Host "Microsoft.ML.OnnxRuntime $Version is not in the NuGet cache."
    Write-Host "Fetch it with the ordinary restore, then run this script again:"
    Write-Host "  .\scripts\restore.ps1"
    exit 1
}

$headers = Join-Path $package "build/native/include"
$binaries = Join-Path $package "runtimes/$Runtime/native"
foreach ($required in @($headers, $binaries)) {
    if (-not (Test-Path -LiteralPath $required)) {
        throw "The package does not contain $required. Is $Version the CPU package?"
    }
}

$destination = Join-Path $repoRoot "artifacts/onnxruntime"
if ((Test-Path -LiteralPath $destination) -and -not $Force) {
    $stampFile = Join-Path $destination "VERSION_NUMBER"
    $staged = if (Test-Path -LiteralPath $stampFile) { (Get-Content -LiteralPath $stampFile -Raw).Trim() } else { "unknown" }
    if ($staged -eq $Version) {
        Write-Host "ONNX Runtime $Version is already staged at $destination."
        exit 0
    }
    Write-Host "Replacing staged ONNX Runtime $staged with $Version."
    Remove-Item -LiteralPath $destination -Recurse -Force
}
elseif (Test-Path -LiteralPath $destination) {
    Remove-Item -LiteralPath $destination -Recurse -Force
}

New-Item -ItemType Directory -Force -Path (Join-Path $destination "include") | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $destination "lib") | Out-Null

Copy-Item -Path (Join-Path $headers "*") -Destination (Join-Path $destination "include") -Recurse -Force
Copy-Item -Path (Join-Path $binaries "*") -Destination (Join-Path $destination "lib") -Recurse -Force

# The licence travels with the binaries. An unlicensed dependency in artifacts/ is exactly
# the provenance gap this repository already carries for its models.
$license = Join-Path $package "LICENSE"
if (Test-Path -LiteralPath $license) {
    Copy-Item -LiteralPath $license -Destination (Join-Path $destination "LICENSE") -Force
}
# Written without a byte order mark: CMake reads this file verbatim, and a BOM would end
# up inside the reported version string.
[System.IO.File]::WriteAllText(
    (Join-Path $destination "VERSION_NUMBER"),
    $Version,
    (New-Object System.Text.UTF8Encoding($false)))

Write-Host "Staged ONNX Runtime $Version ($Runtime) into $destination."
Write-Host "cmake/FindONNXRuntime.cmake resolves it from there with no further configuration."
