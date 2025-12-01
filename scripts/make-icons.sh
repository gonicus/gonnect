#!/usr/bin/env bash
set -e

BASE_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )/.." &> /dev/null && pwd )
ARTWORK_DIR="$BASE_DIR/resources/artwork"

echo -n "Converting images."
magick -background none "$ARTWORK_DIR/gonnect.svg" \
    -density 1200 \
    -define icon:auto-resize=16,48,256 \
    -compress zip \
    "$ARTWORK_DIR/gonnect.ico"

mkdir gonnect.iconset
magick -background none "$ARTWORK_DIR/gonnect.svg" -density 1200 -resize 16x16 gonnect.iconset/icon_16x16.png && echo -n "."
magick -background none "$ARTWORK_DIR/gonnect.svg" -density 1200 -resize 32x32 gonnect.iconset/icon_16x16@2x.png && echo -n "."
magick -background none "$ARTWORK_DIR/gonnect.svg" -density 1200 -resize 32x32 gonnect.iconset/icon_32x32.png && echo -n "."
magick -background none "$ARTWORK_DIR/gonnect.svg" -density 1200 -resize 64x64 gonnect.iconset/icon_32x32@x2.png && echo -n "."
magick -background none "$ARTWORK_DIR/gonnect.svg" -density 1200 -resize 128x128 gonnect.iconset/icon_128x128.png && echo -n "."
magick -background none "$ARTWORK_DIR/gonnect.svg" -density 1200 -resize 256x256 gonnect.iconset/icon_128x128@2x.png && echo -n "."
magick -background none "$ARTWORK_DIR/gonnect.svg" -density 1200 -resize 256x256 gonnect.iconset/icon_256x256.png && echo -n "."
magick -background none "$ARTWORK_DIR/gonnect.svg" -density 1200 -resize 512x512 gonnect.iconset/icon_512x512.png && echo -n "."
magick -background none "$ARTWORK_DIR/gonnect.svg" -density 1200 -resize 1024x1024 gonnect.iconset/icon_1024x1024.png && echo -n "."
iconutil -c icns gonnect.iconset
mv gonnect.icns "$ARTWORK_DIR"
rm -rf gonnect.iconset

echo "OK"
