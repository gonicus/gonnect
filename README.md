<p align="center">
  <img src="media/logo.png" width="410" height="509" alt="GOnnect">
</p>
<!--- TODO: badges --->
<p align="center">
  <a href="https://github.com/gonicus/gonnect/wiki">Documentation</a> |
  <a href="https://github.com/gonicus/gonnect/issues">Issues</a> |
  <a href="https://flathub.org/apps/de.gonicus.gonnect">Install</a>
</p>

---
[![GOnnect workflow](https://github.com/gonicus/gonnect/actions/workflows/gonnect.yml/badge.svg)](https://github.com/gonicus/gonnect/actions/workflows/gonnect.yml)

# Overview

_GOnnect_ is an opinionated, simple, easy to use VoIP client, designed for
Linux / _Flatpak_ based installations. It makes use of various Flatpak
portals and is meant to integrate well into modern Desktop Environments
like _KDE_ or _GNOME_.

![Overview](media/main-screen.png)

What is special about _GOnnect_ is that is meant to be provisioned. For
that reason it has no configuration wizard or settings dialog and
requires a working configuration file in place.

Here's a short feature list:

 * Call forwarding
 * Conference calls with three parties
 * LDAP address sources
 * Busy state for supported sources
 * Configurable identities for outgoing calls
 * Configurable busy on active call
 * Configurable Togglers (i.e. for call queues, CFNL, etc.)
 * Custom audio device profiles or managed by your system
 * System Tray integration for most common functions / favorites / most frequent
   calls
 * [mpris](https://specifications.freedesktop.org/mpris-spec/latest/) for
   stopping other audio sources on incoming calls
 * GNOME Search-Provider support
 * Support for various hardware headsets (i.e. Yealink, Jabra)
 * KRunner search plugin (requires next generation flatpak)
 * Desktop Actions / Autostart
 * Global Keyboard Shortcuts (if supported by your Desktop environment)
 * Keeping the screensaver from droppin' in while on the phone
 * Mime-Type registration for `tel:` / `sip:` URLs
 * Light/Dark mode

# Installing _GOnnect_

_GOnnect_ is distributed via Flathub only. To install it, either visit the
[Flathub page](https://flathub.org) or search for _GOnnect_ in _GNOME-Software_
or KDE's _discover_.

TODO: flathub link

After you have installed _GOnnect_, either adjust the provided
`sample.conf` and place it in `~/.var/app/de.gonicus.gonnect/config/gonnect/99-user.conf`,
or head over to [the documentation](https://github.com/gonicus/gonnect/wiki).

# License

_GOnnect_ is licensed unter the terms of the GNU GENERAL PUBLIC LICENSE
Version 2, or at your opinion any later version.

See `LICENSE` for the full content of the license.

# Development

_GOnnect_ is based on Qt / C++ and requires a set of libraries to be buildable.
There are may ways to achieve a build, but we'll describe just one of them in
the following paragraphs.

## Prerequisites

As we use immutable desktops here at [GONICUS](https://www.gonicus.de) and
development takes place in a dedicated
[distrobox](https://github.com/89luca89/distrobox) for each project, we also use
this procedure for _GOnnect_.

Make sure to have _distrobox_ installed. On _Fedora_ for example run:

```bash
sudo dnf install distrobox
```

After _distrobox_ is installed, create the _distrobox_ for _GOnnect_ development
by running

```bash
distrobox assemble create
```

in the directory of your _GOnnect_ checkout.


## Building 

Assuming you're using the documented _distrobox_ approach above, enter the _distrobox_
and start the ordinary _CMake_ build:

```bash
distrobox enter gonnect
mkdir build && cd build
cmake -GNinja ..
cmake --build . --parallel $(nproc --all)
```

Alternatively you can simply run `qtcreator` inside the _distrobox_ and open the
project as usual be selecting the `CMakeLists.txt`.


## Building the flatpak

As _GOnnect_ is mainly developed for use in _Flatpak_, some features only work in this
kind of environment. If you want to build the _Flatpak_ locally, you can do this by
the following commands on your host shell:

```bash
flatpak uninstall de.gonicus.gonnect
flatpak run --command=flatpak-builder org.flatpak.Builder build --user --install-deps-from=flathub --disable-rofiles-fuse --force-clean --repo=repo resources/flatpak/de.gonicus.gonnect.yml
flatpak --user remote-add --no-gpg-verify gonnect-repo repo
flatpak --user install gonnect-repo de.gonicus.gonnect
```

Make sure to cleanup `repo`, `build` and `.flatpak-builder` to make QtCreator not eat
up your CPU, memory and the whole computer as a desert.
