[CmdletBinding()]
param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Release"
)

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot
$isWindowsHost = [System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform(
    [System.Runtime.InteropServices.OSPlatform]::Windows)
$buildDirectory = if ($isWindowsHost) {
    "artifacts/native/windows-x64"
}
else {
    "artifacts/native/linux-host"
}

Push-Location $repoRoot
try {
    $arguments = @("--test-dir", $buildDirectory, "--output-on-failure")
    if ($isWindowsHost) {
        $arguments += @("-C", $Configuration)
    }

    & ctest @arguments
    if ($LASTEXITCODE -ne 0) {
        throw "Native CTest failed with exit code $LASTEXITCODE"
    }
}
finally {
    Pop-Location
}
