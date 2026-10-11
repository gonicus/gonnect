# Experimental Ubuntu 24.04 AppImage

This separate build targets Ubuntu 24.04 x86_64 with glibc 2.39. It does not
convert the Ubuntu 26.04 package: it rebuilds GOnnect v2.4.1 on Ubuntu 24.04 and
bundles the official Qt 6.10.3 SDK runtime. The host's Qt packages are untouched. A graphical desktop with system GL/EGL/OpenGL
libraries (including Ubuntu's `libopengl0`), ALSA (`libasound2t64`), and drivers
and `gsettings-desktop-schemas` are required. Graphics driver libraries are
intentionally not bundled.
It is not a claim of compatibility with older Ubuntu or every Linux distribution.
Only the SQLite SQL driver is bundled; serial/NMEA positioning is omitted.
The launcher defaults to X11/XWayland (`xcb`), including on GNOME Wayland
sessions. Native Wayland is not yet validated; overriding `QT_QPA_PLATFORM` is
experimental.

```sh
bash packaging/native/build-container.sh appimage --build-arg BUILD_JOBS=4
docker build -f packaging/native/Dockerfile.test-appimage \
  -t gonnect-appimage-test dist/appimage
docker run --rm --name gonnect-appimage-test --network none \
  --memory 2g --cpus 2 --ulimit core=0 gonnect-appimage-test
```

The test launches the actual AppImage with `--appimage-extract-and-run`, under
isolated D-Bus/Xvfb as a regular user, and requires a visible window and clean
exit 0. No host display, audio, credentials or home directory is used. Chromium
sandbox disabling is restricted to that disposable network-disabled test.

## Run locally

```sh
chmod +x GOnnect-2.4.1-ubuntu24.04-x86_64.AppImage
./GOnnect-2.4.1-ubuntu24.04-x86_64.AppImage
# If FUSE is unavailable, no system installation is needed:
./GOnnect-2.4.1-ubuntu24.04-x86_64.AppImage --appimage-extract-and-run
```

Do not disable the Chromium sandbox on your desktop. Ubuntu's AppArmor/user
namespace policy may require a separate narrowly scoped integration decision;
headless container tests cannot validate the host policy. Do not weaken the
system-wide policy just to launch an application.

Provisioning uses `~/.config/gonnect/99-user.conf`, like the native build. To
obtain the example, extract the AppImage in a temporary directory with
`--appimage-extract` and copy
`squashfs-root/usr/share/gonnect/examples/sample.conf` with permissions 600.
Do not run alongside the Flatpak/native package: their D-Bus/application IDs
overlap. No desktop entry or D-Bus service is installed on the host automatically.

## Scope and redistribution

Real calls, audio devices, Jitsi, live-PBX TLS, credential storage, Wayland,
desktop integration and actual FUSE mounting require further tests. Existing
non-Flatpak dummy platform adapters still apply. Bundled Qt, system libraries
and static Conan libraries require AppImage rebuilds for security updates.

This is a local experimental build, not an official upstream binary release.
`gonnect-conan-source.tar.gz` covers the application and Conan dependencies,
**not the bundled Qt SDK or all bundled distro libraries**. Available SDK notice
files and Ubuntu package copyright files are retained inside the image, and builder
package versions are exported. Before redistributing binaries, audit all
licenses, Qt WebEngine's third-party notices and provide exact corresponding
sources for bundled Qt/distro libraries as required. Do not treat this initial
source archive as a complete compliance bundle.

Qt 6.10 supported Linux configurations (including Ubuntu 24.04):
https://doc.qt.io/qt-6.10/linux.html
SDK source archives:
https://download.qt.io/archive/qt/6.10/6.10.3/submodules/

The aqt installer, Conan version and deployment-tool SHA256 values are pinned.
Qt archives use aqt's repository checksum verification. Distribution packages,
transitive Conan revisions and base-image digests are not frozen: bit-for-bit
reproducibility is not claimed. Mutable deployment assets fail closed if the
pinned hashes no longer match.
