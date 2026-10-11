# Experimental native Linux packages

These opt-in container builds create native packages from **the committed source
checkout**. They do not convert a Flatpak, bundle Qt, install anything on the
host, or change the official Flatpak distribution channel.

The initial targets are **Ubuntu 26.04 amd64 (.deb)** and **Fedora 43 x86_64
(.rpm)**, using each distribution's Qt 6.10 packages. These are not packages for
Debian 12/13, Ubuntu 22.04/24.04, RHEL or other releases. Those releases need
separate dependency/toolchain work. The build must match the target release.

GOnnect uses Qt private headers. The generated packages therefore require the
Qt base private ABI version used by the builder, in addition to automatically
scanned ELF dependencies and QML runtime modules mapped by `qmlimportscanner` to their distro packages. Security-only
distro revisions remain upgradeable; rebuild when the upstream Qt private ABI
changes. This is deliberately conservative. The clean runtime smoke test checks
that every resolved system QML module is installed, including lazy-view imports.

## Build

Install Git, Python 3 and Docker with BuildKit support, then run from the
repository root. This implementation targets the release branch (v2.4.1):

```sh
# Commit the packaging branch first. Only versioned HEAD files are built/exported.
# Start with empty dist/deb and dist/rpm directories.
bash packaging/native/build-container.sh deb --build-arg BUILD_JOBS=4
bash packaging/native/build-container.sh rpm --build-arg BUILD_JOBS=4
```

The wrapper uses `git archive HEAD` plus a source manifest, so untracked files,
ignored local secrets and generated presets cannot enter the source archive.
Direct builds from an arbitrary working-directory context are rejected. The
Flatpak-only `resources/flatpak/shared-modules` submodule is not exported; it is
not used by the native build on the release branch.

Each build uses the upstream Conan recipes for non-Qt dependencies, builds the
application and its test target(s), runs CTest with `--no-tests=error`, then
reconfigures with `BUILD_TESTING=OFF` and invokes CPack. Outputs include a package, SHA256SUMS, the Conan dependency
graph (`build-dependencies.json`), third-party license notices and a
`gonnect-corresponding-source.tar.gz` archive with application sources, Conan
recipes, patches and dependency sources. Distribute the source archive alongside
the binary; notices are also installed inside the package. Conan 2.33.0 and base release tags are explicit;
repository packages and Conan transitive revisions are not frozen. Use image
digests and a Conan lockfile if bit-for-bit reproduction is required.

`BASE_IMAGE`, `CONAN_VERSION`, `CONAN_CACHE_ID` and `BUILD_JOBS` are build arguments.
Use a new `CONAN_CACHE_ID` when changing the base release, compiler or system Qt
version; Conan recipes using system Qt do not encode that Qt version in their IDs. Raising the
job limit increases memory use. Bundled static library symbols are hidden to prevent OpenLDAP/OpenSSL symbol
interposition with system libraries. PIE, full RELRO, stack protection and fortify
flags are enabled. Static Conan dependencies, including OpenSSL and PJSIP, require
rebuilding when security fixes are released; distro updates do not update those
embedded copies. This is an additional maintenance burden of these packages.

No privileged container, host cgroup mount,
host D-Bus socket or host display access is needed.

Build logs and installed package versions should be retained with any binary
release. Static Conan dependencies still carry their upstream license and
source-distribution obligations. Audit the retained sources/notices before a
binary release; this mechanism is not a legal-compliance certification. These
experimental binaries are not an upstream-endorsed binary release.

## Install on the matching release

```sh
# Ubuntu 26.04, as a regular user with sudo:
sudo apt install ./dist/deb/gonnect_*.deb

# Fedora 43:
sudo dnf install ./dist/rpm/gonnect-*.rpm
```

Do not use `dpkg --force-depends` or `rpm --nodeps`. Package managers must resolve
the runtime dependencies. Qt is intentionally provided by the distribution.

The desktop entry, icon, AppStream metadata, search provider and D-Bus activation
service are installed under `/usr/share`. Unlike the Flatpak service, the native
D-Bus activation service points at `/usr/bin/gonnect`.

## Configuration and limitations

Do not install this package alongside the Flatpak: both use the same desktop ID
and session D-Bus service name. Choose one installation method.

Copy `/usr/share/gonnect/examples/sample.conf` to
`${XDG_CONFIG_HOME:-$HOME/.config}/gonnect/99-user.conf`, edit it for your SIP
provider and protect credentials:

```sh
install -d -m 700 "${XDG_CONFIG_HOME:-$HOME/.config}/gonnect"
install -m 600 /usr/share/gonnect/examples/sample.conf \
  "${XDG_CONFIG_HOME:-$HOME/.config}/gonnect/99-user.conf"
```

The native package expects a provisioning file and does not change the account
configuration workflow. The Flatpak configuration directory and
Flatpak extensions are not used. Native builds use the existing non-Flatpak
Linux platform implementation; some portal-based features use dummy adapters
(e.g. global shortcuts, background handling and screen inhibition). Matrix
plugins, real SIP calls, audio devices and desktop-specific integration require
separate end-to-end testing. Do not claim Flatpak feature parity.

## Isolated smoke tests

After exporting packages, use the clean runtime images below. They install the
package with dependencies, validate its contents and launch it under an isolated
D-Bus session and virtual X server as an unprivileged user:

```sh
docker build -f packaging/native/Dockerfile.test-deb \
  -t gonnect-native-test-deb dist/deb
docker run --rm --name gonnect-native-test-deb \
  --network none --memory 2g --cpus 2 --ulimit core=0 gonnect-native-test-deb

docker build -f packaging/native/Dockerfile.test-rpm \
  -t gonnect-native-test-rpm dist/rpm
docker run --rm --name gonnect-native-test-rpm \
  --network none --memory 2g --cpus 2 --ulimit core=0 gonnect-native-test-rpm
```

The smoke test disables Chromium's sandbox **only inside this disposable,
network-disabled test container**, because ordinary Docker user-namespace
restrictions can prevent it from starting. Do not use that setting on a real
desktop. No SIP credentials, home directory, audio, host sockets or GUI session
are passed into these test containers. This validates package installation and
GUI startup and clean SIGTERM shutdown, not calls, headsets, Jitsi or credential
storage. A visible window and exit status 0 are both required.

A manual-only `Experimental native packages` GitHub Actions workflow builds both
targets and runs these same smoke tests. It does not publish releases or change
the existing Flatpak CI jobs.

For a fast packaging-only regression check (no Qt build, no package install),
run `bash packaging/native/test-cpack.sh DEB RPM` on a machine with CMake,
`dpkg-dev` and `rpm-build`. The fixture packages contain `/usr/bin/true`, not
GOnnect; they test CPack configuration and rejection guards only.
