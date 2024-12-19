#!/usr/bin/env bash
# Quick hack to generate dark icons based on the fixed primary color of
# breeze icons.
set -e

for svg in *.svg; do
    echo "Tweaking $svg..."
    sed 's/#232629/#d3dae3/g' $svg > dark/$svg
done
