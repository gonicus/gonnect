#!/usr/bin/env bash
set -e

API_TOKEN="${API_TOKEN:-null}"
TMP_OUT=$(mktemp -d)

pushd . &> /dev/null

cmake --preset conan-release -DBUILD_TESTING=OFF -DENABLE_QASSERT=ON

set -eo pipefail
cd build/Release
intercept-build make -j$(nproc --all) 2>&1 | tee /tmp/build.log
set -e

analyze-build -v --output "$TMP_OUT" $ANALYZE_ARGS --plist-html --enable-checker optin.cplusplus.VirtualCall || true

popd &> /dev/null

scripts/process-clang-output.py --exclude '.*/(Qt|qt6|qca|qtkeychain)/.*' --token $API_TOKEN "$TMP_OUT/$(ls -1 $TMP_OUT)"
