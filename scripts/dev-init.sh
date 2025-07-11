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
			echo "unknown argument \"$1\""
			exit 1
			;;
	esac
done

# As this script is called on every Distrobox startup, we use pjsip
# as an indicator, if the script's tasks are already done.
# So we skip the script execution, if pjsip is already installed and
# installation is not forced.
if [ -f /opt/pjsip/include/pjsip.h ] && [ "$FORCE" == "NO" ]; then
	echo "dev-init tasks already done."
	exit 0
fi

TMPDIR=$(mktemp -d)
trap 'rm -rf -- "$TMPDIR"' EXIT

cd "$TMPDIR"
git clone https://github.com/pjsip/pjproject.git && cd pjproject
git checkout 2.15.1
git submodule update --init --recursive
./configure --prefix=/opt/pjsip --disable-video --disable-opus --enable-ext-sound CFLAGS="-fPIC -DPJ_HAS_IPV6=1"
make
sudo make install

if [ ! -e "/run/dbus/system_bus_socket" ]; then
	sudo mkdir -p /run/dbus
	sudo ln -s /run/host/run/dbus/system_bus_socket /run/dbus/system_bus_socket
fi

sudo pip3 install tenacity aiohttp pycairo PyGObject
