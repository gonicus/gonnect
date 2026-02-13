#!/usr/bin/env bash
set -e

TMP_OUT=$(mktemp -d)

pushd . &> /dev/null

cmake --preset conan-release -G "Unix Makefiles" -DBUILD_TESTING=OFF -DENABLE_QASSERT=ON

set -eo pipefail
cd build/Release
intercept-build make -j$(nproc --all) 2>&1 | tee /tmp/build.log
set -e

analyze-build -v --output "$TMP_OUT" $ANALYZE_ARGS --plist-html --enable-checker optin.cplusplus.VirtualCall || true

popd &> /dev/null

scripts/process-clang-output.py --exclude '.*/(Qt|qt6|qca|qtkeychain)/.*' "$TMP_OUT/$(ls -1 $TMP_OUT)"
