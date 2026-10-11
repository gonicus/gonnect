#!/usr/bin/env bash
set -euo pipefail
jobs=${BUILD_JOBS:-4}
[[ "$jobs" =~ ^[1-9][0-9]*$ ]] || exit 2
test -f .native-source-manifest.json || {
    echo 'Use the tracked-source build-container.sh wrapper.' >&2; exit 1;
}
unset FLATPAK_ID
conan profile detect --force
conan config install resources/conan
conan export-dependencies .
conan install . --build=missing -s compiler.cppstd=20 \
    -c tools.cmake.cmaketoolchain:generator=Ninja -c tools.build:jobs="$jobs" \
    -c tools.build:download_source=True \
    -c 'tools.build:cflags=["-fstack-protector-strong","-D_FORTIFY_SOURCE=3"]' \
    -c 'tools.build:cxxflags=["-fstack-protector-strong","-D_FORTIFY_SOURCE=3"]' \
    -c 'tools.build:exelinkflags=["-Wl,-z,relro,-z,now"]' \
    --format=json > build-dependencies.json
mkdir -p /out
python3 packaging/native/collect-sources.py build-dependencies.json /out
# Qt SDK sources are separate; this archive covers GOnnect and its Conan dependencies.
mv /out/gonnect-corresponding-source.tar.gz /out/gonnect-conan-source.tar.gz
cmake --preset conan-release . -DCMAKE_INSTALL_PREFIX=/usr \
    -DENABLE_NATIVE_PACKAGING=ON -DBUILD_TESTING=ON -DENABLE_LTO=OFF
cmake --build --preset conan-release --parallel "$jobs"
QT_QPA_PLATFORM=offscreen ctest --test-dir build/Release/tests \
    --output-on-failure --no-tests=error --output-log /out/ctest.log
cp build/Release/tests/Testing/Temporary/LastTest.log /out/ctest-details.log
cmake --preset conan-release . -DBUILD_TESTING=OFF
cmake --build --preset conan-release --parallel "$jobs"
DESTDIR=/src/AppDir cmake --install build/Release

# Verify downloaded deployment tools before executing them. Continuous assets
# are mutable: fail closed if upstream replaces one; review and update the pin.
mkdir -p /opt/deploy/bin
fetch_tool() {
    local name=$1 repo=$2 sha=$3
    curl --fail --location --retry 3 \
        "https://github.com/${repo}/releases/download/continuous/${name}-x86_64.AppImage" \
        -o "/opt/deploy/${name}.AppImage"
    echo "$sha  /opt/deploy/${name}.AppImage" | sha256sum -c -
    chmod +x "/opt/deploy/${name}.AppImage"
    mkdir -p "/opt/deploy/${name}"
    (cd "/opt/deploy/${name}" && "../${name}.AppImage" --appimage-extract >/dev/null)
    printf '#!/bin/sh\nexec /opt/deploy/%s/squashfs-root/AppRun "$@"\n' "$name" \
        > "/opt/deploy/bin/${name}"
    chmod +x "/opt/deploy/bin/${name}"
}
fetch_tool linuxdeploy linuxdeploy/linuxdeploy \
    8aea8da0f7f7039d2a2cecb14657d752a222a5e1d3825caeef186c82f751cdd1
fetch_tool linuxdeploy-plugin-qt linuxdeploy/linuxdeploy-plugin-qt \
    cfc1055b2b9dbc08412b579f20990b7b41a17b61beaa5847dc9477c96c9e9617
fetch_tool appimagetool AppImage/appimagetool \
    95cbe7cce9717fce90c484e34052ee7c7f1d7635b33c12525b4776826a7d29b6
curl --fail --location --retry 3 \
    https://github.com/AppImage/type2-runtime/releases/download/continuous/runtime-x86_64 \
    -o /opt/deploy/runtime-x86_64
echo '156f4bdbde9c52d01814600013e0a273f0118dc2de98975f3c8c63427ec79074  /opt/deploy/runtime-x86_64' \
    | sha256sum -c -
export PATH="/opt/deploy/bin:$PATH"
export QMAKE="$QT_ROOT/bin/qmake"
export QML_SOURCES_PATHS=/src/src
export EXTRA_QT_MODULES=svg
export EXTRA_PLATFORM_PLUGINS=libqwayland.so
export LD_LIBRARY_PATH="$QT_ROOT/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
# GOnnect only opens SQLite databases. Do not bundle unused SDK SQL drivers
# (PostgreSQL, ODBC, etc.) or introduce their unrelated runtime dependencies.
find "$QT_ROOT/plugins/sqldrivers" -type f ! -name libqsqlite.so -delete
linuxdeploy --appdir AppDir --executable AppDir/usr/bin/gonnect \
    --desktop-file AppDir/usr/share/applications/de.gonicus.gonnect.desktop \
    --icon-file AppDir/usr/share/icons/hicolor/scalable/apps/de.gonicus.gonnect.svg \
    --library /usr/lib/x86_64-linux-gnu/libsoftokn3.so \
    --library /usr/lib/x86_64-linux-gnu/libfreebl3.so \
    --library /usr/lib/x86_64-linux-gnu/libfreeblpriv3.so \
    --library /usr/lib/x86_64-linux-gnu/libnssckbi.so \
    --plugin qt > /out/deployment.log 2>&1 || { tail -150 /out/deployment.log; exit 1; }

# Use an explicit portable entry point. Default to the X11/XWayland path tested
# below rather than silently selecting an untested native Wayland integration.
rm -f AppDir/AppRun
cp packaging/native/AppRun AppDir/AppRun
chmod +x AppDir/AppRun
rm -f AppDir/usr/share/dbus-1/services/de.gonicus.gonnect.service
find AppDir -name '*.desktop' -type f -exec sed -i \
    's/^DBusActivatable=true$/DBusActivatable=false/' '{}' +

# Retain SDK license notices where provided, plus installed Ubuntu package notices.
mkdir -p AppDir/usr/share/licenses/gonnect/qt-sdk AppDir/usr/share/licenses/gonnect/ubuntu
find /opt/Qt -type f \( -iname 'LICENSE*' -o -iname 'COPYING*' -o -iname 'NOTICE*' \) \
    -exec cp --parents '{}' AppDir/usr/share/licenses/gonnect/qt-sdk/ \;
find /usr/share/doc -mindepth 2 -maxdepth 2 -name copyright -type f \
    -exec cp --parents '{}' AppDir/usr/share/licenses/gonnect/ubuntu/ \;
cp packaging/native/APPIMAGE.md AppDir/usr/share/licenses/gonnect/BUILD-NOTES.md
cp build-dependencies.json /out/
dpkg-query -W > /out/build-system-packages.txt
nm -D --defined-only AppDir/usr/bin/gonnect > /out/exported-symbols.txt
if grep -Eq ' (SSL|EVP|CRYPTO|ldap|ber)_' /out/exported-symbols.txt; then
    echo 'Static library exports detected' >&2; exit 1
fi
readelf -h -l -d AppDir/usr/bin/gonnect > /out/elf-audit.txt
grep -q 'Type:.*DYN' /out/elf-audit.txt
grep -q GNU_RELRO /out/elf-audit.txt
grep -Eq 'BIND_NOW|FLAGS_1.*NOW' /out/elf-audit.txt
ARCH=x86_64 appimagetool --no-appstream --runtime-file /opt/deploy/runtime-x86_64 \
    AppDir /out/GOnnect-2.4.1-ubuntu24.04-x86_64.AppImage \
    > /out/appimagetool.log 2>&1 || { tail -100 /out/appimagetool.log; exit 1; }
cp packaging/native/smoke-appimage.sh /out/
# SHA256SUMS is excluded from the input list.
# shellcheck disable=SC2094
(cd /out && find . -maxdepth 1 -type f ! -name SHA256SUMS -exec sha256sum {} + > SHA256SUMS)
