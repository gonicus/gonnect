#!/usr/bin/env bash
set -euo pipefail

format=${1:?Usage: build-container.sh deb|rpm|appimage [docker build arguments...]}
shift
case "$format" in deb|rpm|appimage) ;; *) echo 'Expected deb, rpm or appimage' >&2; exit 2 ;; esac
root=$(git rev-parse --show-toplevel)
cd "$root"
if [[ -n $(git status --porcelain --untracked-files=no) ]]; then
    echo 'Commit your changes first; only HEAD is exported into the build context.' >&2
    exit 1
fi
output="$root/dist/$format"
if [[ -d "$output" && -n $(find "$output" -mindepth 1 -maxdepth 1 -print -quit) ]]; then
    echo "Output is not empty: $output; move previous artifacts aside first." >&2
    exit 1
fi
scratch=$(mktemp -d)
trap 'rm -rf "$scratch"' EXIT
git archive --format=tar HEAD > "$scratch/source.tar"
python3 - "$scratch/source.tar" "$(git rev-parse HEAD)" <<'PYTHON'
import io
import json
import sys
import tarfile
with tarfile.open(sys.argv[1], "a") as archive:
    manifest = {"revision": sys.argv[2], "files": [item.name for item in archive.getmembers()]}
    data = json.dumps(manifest).encode()
    entry = tarfile.TarInfo(".native-source-manifest.json")
    entry.size = len(data)
    entry.mode = 0o644
    archive.addfile(entry, io.BytesIO(data))
PYTHON
mkdir -p "$output"
docker build -f "packaging/native/Dockerfile.$format" --target artifacts \
    --output "type=local,dest=$output" "$@" - < "$scratch/source.tar"
