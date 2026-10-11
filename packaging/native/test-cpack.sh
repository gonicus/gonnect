#!/usr/bin/env bash
set -euo pipefail

# A small fixture exercises packaging without compiling Qt or installing packages.
root=$(cd "$(dirname "$0")/../.." && pwd)
fixture=$(mktemp -d)
trap 'rm -rf "$fixture"' EXIT
mkdir -p "$fixture/docs/modules/ROOT/examples"
cp "$root/LICENSE" "$fixture/"
cp "$root/docs/modules/ROOT/examples/sample.conf" "$fixture/docs/modules/ROOT/examples/"
cat > "$fixture/CMakeLists.txt" <<CMAKE
cmake_minimum_required(VERSION 3.26)
project(NativePackageFixture VERSION 1.2.3 LANGUAGES NONE)
set(GOnnect_VERSION 1.2.3)
include("$root/cmake/Versioning.cmake")
if(NOT "\${VERSION}" STREQUAL "1.2.3")
    message(FATAL_ERROR "Version fallback failed without .git")
endif()
set(LINUX TRUE)
set(APPLICATION_NAME GOnnect)
set(APPLICATION_EXECUTABLE gonnect)
set(APPLICATION_VENDOR "GONICUS GmbH")
set(APPLICATION_URL "https://www.gonnect-project.org")
install(PROGRAMS /usr/bin/true DESTINATION bin RENAME gonnect)
include("$root/packaging/CMakeLists.txt")
CMAKE
cmake -S "$fixture" -B "$fixture/default" -DCMAKE_INSTALL_PREFIX=/usr
if grep -q 'CPACK_DEBIAN_PACKAGE_NAME "gonnect"' "$fixture/default/CPackConfig.cmake"; then
    echo 'Native packaging unexpectedly enabled by default' >&2
    exit 1
fi
cmake -S "$fixture" -B "$fixture/native" -DCMAKE_INSTALL_PREFIX=/usr -DENABLE_NATIVE_PACKAGING=ON
for generator in "$@"; do
    case "$generator" in DEB|RPM) ;; *) exit 2 ;; esac
    (cd "$fixture/native" && cpack -G "$generator")
done
if cmake -S "$fixture" -B "$fixture/prefix" -DCMAKE_INSTALL_PREFIX=/opt/gonnect -DENABLE_NATIVE_PACKAGING=ON; then
    echo 'Native packaging accepted a non-/usr prefix' >&2
    exit 1
fi
if cmake -S "$fixture" -B "$fixture/flatpak" -DCMAKE_INSTALL_PREFIX=/usr -DENABLE_NATIVE_PACKAGING=ON -DFLATPAK=de.gonicus.gonnect; then
    echo 'Native packaging accepted a Flatpak build' >&2
    exit 1
fi
echo 'CPack opt-in and native build guards passed.'
