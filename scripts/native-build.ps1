[CmdletBinding()]
param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Release"
)

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot
$isWindowsHost = [System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform(
    [System.Runtime.InteropServices.OSPlatform]::Windows)

Push-Location $repoRoot
try {
    if ($isWindowsHost) {
        & cmake --preset windows-x64
        if ($LASTEXITCODE -ne 0) {
            throw "Native CMake configure failed with exit code $LASTEXITCODE"
        }

        & cmake --build "artifacts/native/windows-x64" --config $Configuration
    }
    else {
        $buildDirectory = "artifacts/native/linux-host"
        & cmake -S . -B $buildDirectory -G Ninja `
            "-DCMAKE_BUILD_TYPE=$Configuration" `
            -DNVISION_BUILD_TESTS=ON
        if ($LASTEXITCODE -ne 0) {
            throw "Native CMake configure failed with exit code $LASTEXITCODE"
        }

        & cmake --build $buildDirectory
    }

    if ($LASTEXITCODE -ne 0) {
        throw "Native CMake build failed with exit code $LASTEXITCODE"
    }
}
finally {
    Pop-Location
}
