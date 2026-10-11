#!/usr/bin/env bash
set -euo pipefail
export XDG_RUNTIME_DIR
XDG_RUNTIME_DIR=$(mktemp -d)
chmod 700 "$XDG_RUNTIME_DIR"
trap 'rm -rf "$XDG_RUNTIME_DIR"' EXIT
export QTWEBENGINE_DISABLE_SANDBOX=1
export QTWEBENGINE_CHROMIUM_FLAGS=--disable-gpu
# Test the actual AppImage's FUSE-free execution, not an unpackaged build tree.
set +e
# shellcheck disable=SC2016
dbus-run-session -- xvfb-run -a bash -c '
    /opt/gonnect/GOnnect.AppImage --appimage-extract-and-run > /tmp/gonnect-appimage.log 2>&1 &
    launcher=$!
    app=""
    trap "kill -KILL \${app:-$launcher} 2>/dev/null || true; wait $launcher 2>/dev/null || true" EXIT
    found=0
    for attempt in {1..20}; do
        if window=$(xdotool search --onlyvisible --class "[Gg][Oo]nnect" 2>/dev/null | head -n1) && [[ -n "$window" ]]; then
            app=$(xdotool getwindowpid "$window")
            [[ "$app" =~ ^[0-9]+$ ]] || exit 1
            found=1
            break
        fi
        kill -0 "$launcher" 2>/dev/null || break
        sleep 1
    done
    [[ "$found" == 1 ]] || { echo "No visible GOnnect window" >&2; exit 1; }
    executable=$(readlink /proc/"$app"/exe)
    case "$executable" in */appimage_extracted_*/usr/bin/gonnect) ;; *) echo "Unexpected GUI executable" >&2; exit 1 ;; esac
    printf "%s\n" "$executable"
    sleep 10
    kill -0 "$app" 2>/dev/null || exit 1
    kill -TERM "$app" || exit 1
    for attempt in {1..10}; do
        kill -0 "$launcher" 2>/dev/null || break
        sleep 1
    done
    kill -0 "$launcher" 2>/dev/null && { echo "Shutdown timed out" >&2; exit 1; }
    wait "$launcher"
    status=$?
    [[ "$status" == 0 ]] || { echo "AppImage exit status: $status" >&2; exit 1; }
'
result=$?
set -e
cat /tmp/gonnect-appimage.log
[[ "$result" == 0 ]] || exit 1
if grep -Ei 'failed to load component|module .* is not installed|cannot load library|error while loading shared libraries|segmentation fault|dumped core|aborted|fatal|is not a type|qrc:.*Error' /tmp/gonnect-appimage.log; then
    echo 'AppImage startup errors detected' >&2; exit 1
fi
echo 'Actual AppImage extraction, visible GUI and clean shutdown passed on Ubuntu 24.04.'
