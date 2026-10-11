#!/usr/bin/env bash
set -euo pipefail

ldd /usr/bin/gonnect > /tmp/gonnect-ldd.log
cat /tmp/gonnect-ldd.log
if grep -q 'not found' /tmp/gonnect-ldd.log; then
    echo 'Missing shared libraries' >&2
    exit 1
fi
python3 - <<'PYTHON'
import json
from pathlib import Path
modules = json.loads(Path("/usr/local/share/gonnect-qml-runtime.json").read_text())
assert modules, "No QML imports retained"
for module in modules:
    path = Path(module["path"])
    if module.get("kind") != "plugin":
        path = path / "qmldir"
    assert path.is_file(), module["name"]
print(f"Verified {len(modules)} QML modules and runtime plugins")
PYTHON
test -s /usr/share/icons/hicolor/scalable/apps/de.gonicus.gonnect.svg
test -s /usr/share/gonnect/examples/sample.conf
grep -Fx 'Exec=/usr/bin/gonnect' /usr/share/dbus-1/services/de.gonicus.gonnect.service
desktop-file-validate /usr/share/applications/de.gonicus.gonnect.desktop
export XDG_RUNTIME_DIR
XDG_RUNTIME_DIR=$(mktemp -d)
chmod 700 "$XDG_RUNTIME_DIR"
trap 'rm -rf "$XDG_RUNTIME_DIR"' EXIT
export QTWEBENGINE_DISABLE_SANDBOX=1
export QTWEBENGINE_CHROMIUM_FLAGS=--disable-gpu

set +e
# Variables below are expanded by the child shell inside the virtual display.
# shellcheck disable=SC2016
dbus-run-session -- xvfb-run -a bash -c '
    gonnect > /tmp/gonnect-startup.log 2>&1 &
    app=$!
    trap "kill -KILL \"$app\" 2>/dev/null || true; wait \"$app\" 2>/dev/null || true" EXIT
    found=0
    for attempt in {1..10}; do
        if xdotool search --onlyvisible --class "[Gg][Oo]nnect" >/dev/null 2>&1; then
            found=1
            break
        fi
        sleep 1
    done
    if [[ "$found" != 1 ]]; then
        echo "No visible GOnnect window appeared" >&2
        exit 1
    fi
    sleep 10
    if ! kill -0 "$app" 2>/dev/null; then
        wait "$app"
        echo "GOnnect exited before the shutdown test" >&2
        exit 1
    fi
    kill -TERM "$app" || exit 1
    for attempt in {1..10}; do
        kill -0 "$app" 2>/dev/null || break
        sleep 1
    done
    if kill -0 "$app" 2>/dev/null; then
        echo "GOnnect did not shut down after SIGTERM" >&2
        exit 1
    fi
    wait "$app"
    status=$?
    if [[ "$status" != 0 ]]; then
        echo "GOnnect shutdown failed: $status" >&2
        exit 1
    fi
'

result=$?
set -e
cat /tmp/gonnect-startup.log
if [[ "$result" != 0 ]]; then
    echo "GUI startup/shutdown test failed: $result" >&2
    exit 1
fi
if grep -Ei 'failed to load component|module .* is not installed|cannot load library|error while loading shared libraries|segmentation fault|dumped core|aborted|fatal|unavailable|is not a type|qrc:.*Error' /tmp/gonnect-startup.log; then
    echo 'Startup errors detected' >&2
    exit 1
fi
echo 'Package installation, dependencies, desktop metadata and GUI startup passed.'
