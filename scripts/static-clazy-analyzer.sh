#!/usr/bin/env bash
set -eo pipefail
API_TOKEN="${API_TOKEN:-null}"

pushd .

rm -rf build; mkdir build; cd build

export CC=/usr/bin/clang
export CXX=/usr/bin/clang++
export CLAZY_IGNORE_DIRS="(.*qmltyperegistrations.*|.*/tests/.*|.*/\\.rcc/.*|.*/build/src/.*)"
export CLAZY_CHECKS="level0,level1,qt6-header-fixes,no-no-module-include,no-lambda-unique-connection"
export CLAZY_NO_WERROR=ON

cmake -GNinja -DCMAKE_BUILD_TYPE=Release -DBUILD_DEPENDENCIES=ON -DENABLE_CLAZY=ON -DCMAKE_PREFIX_PATH="$EXT_BASE/pjproject;$EXT_BASE/qca/lib/cmake" ..
cmake --build . --parallel $(nproc --all) 2>&1 | tee /tmp/build.log

popd

scripts/process-clazy-output.py --token $API_TOKEN /tmp/build.log
