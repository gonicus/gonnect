#!/usr/bin/env bash
set -e

BASE_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )/.." &> /dev/null && pwd )
ARTWORK_DIR="$BASE_DIR/resources/artwork"

echo -n "Converting images."
magick "$ARTWORK_DIR/gonnect.svg" \
    -density 1200 \
    -background transparent \
    -define icon:auto-resize=16,48,256 \
    -compress zip \
    "$ARTWORK_DIR/gonnect.ico"

ICNS=""
for res in 16 32 128 256 512 1024; do
    echo -n "."
    ICNS="$ICNS $HOME/tmp/icns-$res.png"
    magick "$ARTWORK_DIR/gonnect.svg" -density 1200 -background transparent -resize ${res}x${res} ~/tmp/icns-$res.png
done

echo -n "."
png2icns "$ARTWORK_DIR/gonnect.icns" $ICNS &> /dev/null

echo "OK"
