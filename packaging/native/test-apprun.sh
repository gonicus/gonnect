#!/usr/bin/env bash
set -euo pipefail
root=$(CDPATH='' cd -- "$(dirname -- "$0")" && pwd)
scratch=$(mktemp -d)
trap 'rm -rf "$scratch"' EXIT
# Exercise the launcher without Qt, a display, audio devices or SIP credentials.
mkdir -p "$scratch/usr/bin"
cp "$root/AppRun" "$scratch/AppRun"
chmod +x "$scratch/AppRun"
cat > "$scratch/usr/bin/gonnect" <<'SH'
#!/bin/sh
set -eu
test "$QT_AUDIO_BACKEND" = "$TEST_EXPECT_BACKEND"
test "$QT_QPA_PLATFORM" = "$TEST_EXPECT_PLATFORM"
test "$GSETTINGS_SCHEMA_DIR" = "$TEST_APPDIR/usr/share/glib-2.0/schemas"
test "${LD_LIBRARY_PATH+x}" != x
test "${QT_PLUGIN_PATH+x}" != x
test "${QML2_IMPORT_PATH+x}" != x
test "$#" -eq 2
test "$1" = 'argument with spaces'
test "$2" = '--example'
SH
chmod +x "$scratch/usr/bin/gonnect"
export TEST_APPDIR="$scratch"
export LD_LIBRARY_PATH=/nonexistent QT_PLUGIN_PATH=/nonexistent
export QML2_IMPORT_PATH=/nonexistent
unset QT_AUDIO_BACKEND QT_QPA_PLATFORM
TEST_EXPECT_BACKEND=pulseaudio TEST_EXPECT_PLATFORM=xcb \
    "$scratch/AppRun" 'argument with spaces' --example
QT_AUDIO_BACKEND='' QT_QPA_PLATFORM='' \
    TEST_EXPECT_BACKEND=pulseaudio TEST_EXPECT_PLATFORM=xcb \
    "$scratch/AppRun" 'argument with spaces' --example
QT_AUDIO_BACKEND=pipewire QT_QPA_PLATFORM=wayland \
    TEST_EXPECT_BACKEND=pipewire TEST_EXPECT_PLATFORM=wayland \
    "$scratch/AppRun" 'argument with spaces' --example
echo 'AppRun: defaults, overrides, runtime paths and arguments passed'
