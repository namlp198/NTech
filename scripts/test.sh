#!/bin/sh
set -eu

repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$repo_root"
dotnet test NTech.NVision.sln --configuration Release --no-build --no-restore
