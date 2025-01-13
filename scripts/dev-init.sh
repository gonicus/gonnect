#!/usr/bin/env bash
set -e

# Parse command line arguments
FORCE=NO
while [[ $# -gt 0 ]]; do
	case $1 in
		-f|--force)
			FORCE=YES
			shift
			;;
		*)
			echo "unknow argument \"$1\""
			exit 1
			;;
	esac
done

# Skip script execution, if pjsip is already installed and
# installation is not forced.
if [ -f /opt/pjsip/include/pjsip.h ] && [ "$FORCE" == "NO" ]; then
	echo "pjsip already installed."
	exit 0
fi

PROJECT_ROOT="/tmp/gonnect/build"
mkdir -p $PROJECT_ROOT

TMPDIR=$(mktemp -d -p $PROJECT_ROOT)
trap 'rm -rf -- "$TMPDIR"' EXIT

cd "$TMPDIR"
git clone https://github.com/pjsip/pjproject.git && cd pjproject
git submodule update --init --recursive
git checkout 2.15.1
./configure --prefix=/opt/pjsip --disable-video --disable-opus --enable-ext-sound CFLAGS="-fPIC -DPJ_HAS_IPV6=1"
make
sudo make install
