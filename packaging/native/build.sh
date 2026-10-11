#!/usr/bin/env bash
set -euo pipefail

format=${1:?Usage: build.sh DEB|RPM}
case "$format" in
    DEB|RPM) ;;
    *) echo 'Expected DEB or RPM' >&2; exit 2 ;;
esac
jobs=${BUILD_JOBS:-4}
[[ "$jobs" =~ ^[1-9][0-9]*$ ]] || { echo 'BUILD_JOBS must be a positive integer' >&2; exit 2; }

unset FLATPAK_ID
test -f .native-source-manifest.json || {
    echo 'Use packaging/native/build-container.sh to export a tracked-only source context.' >&2
    exit 1
}
conan profile detect --force
conan config install resources/conan
conan export-dependencies .
conan install . --build=missing -s compiler.cppstd=20 \
    -c tools.cmake.cmaketoolchain:generator=Ninja -c tools.build:jobs="$jobs" \
    -c tools.build:download_source=True \
    -c 'tools.build:cflags=["-fstack-protector-strong","-D_FORTIFY_SOURCE=3"]' \
    -c 'tools.build:cxxflags=["-fstack-protector-strong","-D_FORTIFY_SOURCE=3"]' \
    -c 'tools.build:exelinkflags=["-Wl,-z,relro,-z,now"]' \
    --format=json > /src/build-dependencies.json

mkdir -p /out
python3 packaging/native/collect-sources.py build-dependencies.json /out

# Qt private headers are used; pin the upstream ABI, not security-only distro revisions.
if [[ "$format" == DEB ]]; then
    qt_provides=$(dpkg-query -W -f='${Provides}' libqt6core6t64)
    qt_version=$(printf '%s' "$qt_provides" | python3 -c 'import re,sys; print(re.search(r"qt6-base-private-abi \(= ([^)]+)\)", sys.stdin.read())[1])')
    dependency_key=CPACK_DEBIAN_PACKAGE_DEPENDS
    abi_dependency="qt6-base-private-abi (= ${qt_version}), qt6-wayland"
else
    qt_version=$(rpm -q --qf '%{VERSION}' qt6-qtbase)
    dependency_key=CPACK_RPM_PACKAGE_REQUIRES
    abi_dependency="qt6-qtbase$(rpm --eval '%{?_isa}') = ${qt_version}, qt6-qtwayland"
fi
cmake --preset conan-release . -DCMAKE_INSTALL_PREFIX=/usr \
    -DENABLE_NATIVE_PACKAGING=ON -DBUILD_TESTING=ON -DENABLE_LTO=OFF \
    -D "${dependency_key}=${abi_dependency}"
cmake --build --preset conan-release --parallel "$jobs"
QT_QPA_PLATFORM=offscreen ctest --test-dir build/Release/tests --output-on-failure --no-tests=error
# Resolve all QML imports, not only the modules loaded by the startup smoke test.
"$(qtpaths6 --query QT_INSTALL_LIBEXECS)/qmlimportscanner" \
    -rootPath src -importPath build/Release/src \
    -importPath "$(qtpaths6 --query QT_INSTALL_QML)" > /out/qml-imports.json
qml_dependencies=$(python3 packaging/native/qml-dependencies.py "$format" \
    /out/qml-imports.json /out/qml-runtime.json)
# Do not distribute an application configured with test-only definitions.
cmake --preset conan-release . -DBUILD_TESTING=OFF \
    -D "${dependency_key}=${abi_dependency}, ${qml_dependencies}"
cmake --build --preset conan-release --parallel "$jobs"
mkdir -p /out
nm -D --defined-only build/Release/src/gonnect > /out/exported-symbols.txt
if grep -Eq ' (SSL|EVP|CRYPTO|ldap|ber)_' /out/exported-symbols.txt; then
    echo 'Bundled library symbols are still exported' >&2
    exit 1
fi
readelf -h -l -d build/Release/src/gonnect > /out/elf-audit.txt
cp build-dependencies.json /out/
cp packaging/native/smoke.sh /out/
(cd build/Release && cpack -G "$format" -B /out)
rm -rf /out/_CPack_Packages
# SHA256SUMS is explicitly excluded from the input file list.
# shellcheck disable=SC2094
(cd /out && find . -maxdepth 1 -type f ! -name SHA256SUMS -exec sha256sum {} + > SHA256SUMS)
