#!/usr/bin/env bash
set -eo pipefail

export CLAZY_IGNORE_DIRS="(.*qmltyperegistrations.*|.*/tests/.*|.*/\\.rcc/.*|.*/build/Release/src/.*)"
export CLAZY_CHECKS="level0,level1,qt6-header-fixes,no-no-module-include,no-lambda-unique-connection"
export CLAZY_NO_WERROR=ON

cmake -GNinja --preset conan-release -DENABLE_CLAZY=ON
cmake --build --preset conan-release --parallel $(nproc --all) 2>&1 | tee /tmp/build.log

scripts/process-clazy-output.py /tmp/build.log
