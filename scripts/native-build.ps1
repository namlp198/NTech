[CmdletBinding()]
param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Release"
)

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot
$isWindowsHost = [System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform(
    [System.Runtime.InteropServices.OSPlatform]::Windows)

# OpenCV is located by the standard CMake search, which honours the OpenCV_DIR
# environment variable. Setting it is how a machine opts its build into the OpenCV-backed
# vision tools; no path to an OpenCV installation is stored in this repository.
$openCvArguments = @()
if ($env:OpenCV_DIR) {
    $openCvArguments += "-DNVISION_ENABLE_OPENCV=ON"
    Write-Verbose "Building the OpenCV-backed vision tools using OpenCV_DIR=$($env:OpenCV_DIR)"
}

Push-Location $repoRoot
try {
    if ($isWindowsHost) {
        & cmake --preset windows-x64 @openCvArguments
        if ($LASTEXITCODE -ne 0) {
            throw "Native CMake configure failed with exit code $LASTEXITCODE"
        }

        & cmake --build "artifacts/native/windows-x64" --config $Configuration
    }
    else {
        $buildDirectory = "artifacts/native/linux-host"
        & cmake -S . -B $buildDirectory -G Ninja `
            "-DCMAKE_BUILD_TYPE=$Configuration" `
            -DNVISION_BUILD_TESTS=ON `
            @openCvArguments
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
