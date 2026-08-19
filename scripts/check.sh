#!/bin/sh
set -eu

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
"$script_dir/restore.sh"
"$script_dir/build.sh"
"$script_dir/test.sh"

repo_root=$(CDPATH= cd -- "$script_dir/.." && pwd)
cd "$repo_root"
dotnet format NTech.NVision.sln --verify-no-changes --no-restore
