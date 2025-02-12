#!/usr/bin/env bash
set -e

API_TOKEN="${API_TOKEN:-null}"
TMP_OUT=$(mktemp -d)

pushd . &> /dev/null

rm -rf build; mkdir build; cd build

export CC=/usr/bin/clang
export CXX=/usr/bin/clang++

cmake -DBUILD_TESTING=OFF -DBUILD_DEPENDENCIES=ON -DENABLE_QASSERT=ON -DCMAKE_PREFIX_PATH="$EXT_BASE/pjproject;$EXT_BASE/qca/lib/cmake" ..

set -eo pipefail
intercept-build make -j$(nproc --all) 2>&1 | tee /tmp/build.log
set -e

analyze-build -v --output "$TMP_OUT" $ANALYZE_ARGS --plist-html --enable-checker optin.cplusplus.VirtualCall || true

popd &> /dev/null

scripts/process-clang-output.py --exclude '.*/(Qt|qt6)/.*' --token $API_TOKEN "$TMP_OUT/$(ls -1 $TMP_OUT)"
