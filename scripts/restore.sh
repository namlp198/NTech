#!/bin/sh
set -eu

repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$repo_root"
dotnet restore NTech.NVision.sln --configfile NuGet.Config
