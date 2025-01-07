#!/usr/bin/env bash
set -e

PROJECT_ROOT="/tmp/gossip/build"
mkdir -p $PROJECT_ROOT

TMPDIR=$(mktemp -d -p $PROJECT_ROOT)
trap 'rm -rf -- "$TMPDIR"' EXIT

cd $TMPDIR
git clone https://github.com/pjsip/pjproject.git && cd pjproject
git submodule update --init --recursive
git checkout 2.15.1
./configure --prefix=/opt/pjsip --disable-video --disable-opus --enable-ext-sound CFLAGS="-fPIC -DPJ_HAS_IPV6=1"
make
sudo make install
