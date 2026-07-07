# GOnnect description for AI agents

This document describes the GOnnect program for usage by AI agents.

GOnnect is a FOSS UC (unified communication) client for Linux and Windows desktops.

It currently supports:

1. SIP telephony
2. Jitsi Meet integration
3. Matrix Chat


## Main entry points

* `src/CMakeLists.txt` is the main cmake file for the main application (although another file is top level to integrate sub folders)
* `src/Application.h` constructs and set ups the main application
* `src/ui/GonnectWindow.qml` is the main qml window file (used by `src/Main.qml` as the top-most loaded file)
* `src/ui/components/MainTabBar.qml` is the main navigation button bar for the application
* `src/platform/NotificationManager.h` manages all desktop notifications, which are a main feature of the app
* `src/GlobalCallState.h` aggregates the states of `src/ICallState.h` objects
* `src/presences/GlobalStateAggregator.h` aggregates presence state of different providers
* `src/GlobalMuteState.h` is the global point for audio mute and tries to communicate between own headset support, os and providers such as Jitsi Meet.


## SIP

GOnnect supports phone calls via SIP, including functionalities like call forwarding and ad-hoc threesome conferences.

Main classes:

* `src/sip/SIPAccount.h` defines registration of a SIP account and options
* `src/sip/SIPCall.h` representing a single call
* `src/sip/SIPCallManager.h` creates, manages and deletes calls
* `qrc/sip/SIPBuddy.h` represents a SIP buddy which can be used to retrieve presence information of another user

Main UI components:
* `src/ui/components/pages/Call.qml` is the main page for a call
* `src/ui/components/pages/CallDetails.qml` and `src/ui/components/pages/CallDetails.qml` show the details of an ongoing call
* `src/ui/components/pages/CallButtonBar.qml` is the main button bar for actions on the call
* `src/ui/components/pages/CallSideBar.qml` holds additional views and infos for a call


## HID

GOnnect supports various busylights, videolights and headsets via USB HID. Headsets that follow the *Telephony Usage Page* are supported.
Additional features are addressed for headsets that follow the Microsoft Teams specification (i.e. display support). Home of the USB
related implementation is `src/usb`.

* `src/usb/ReportDescriptorParser.h` is the descriptor parser
* `src/usb/busylight` keeps the busylight/videolight implementations
* `src/usb/HeadsetDevice.h` and `src/usb/HeadsetDeviceProxy.h` deal with the headsets


## Jitsi Meet

GOnnect integrates Jitsi Meet by using it via its iframe API with the Qt/Qml web engine.

* The main class for the Jitsi Meet integration is `src/ui/JitsiConnector.cpp`.
* The main UI file is `src/ui/pages/Conference.qml`, controlled by `src/ui/components/ConferenceButtonBar.qml`.
* Users can seamlessly upgrade a SIP call into an own Jitsi Meet room, mainly by using functions such as "screen share" in `src/ui/components/pages/CallButtonBar.qml`.


## Matrix chat

GOnnect integrates a Matrix chat, by utilizing another CLI tool by which it communicates via IPC with protobuf messages.

* The protobuf definitions are in `external/protos/chat.proto`.
* The main class glueing the communication is `src/chat/IpcDispatcher.h`.
* UI entry point is in `src/ui/components/pages/Chats.qml`.
* The list of chat rooms is drawn in `src/ui/components/chat/ChatRoomList.qml`, fed by `src/chat/ChatRoomModel.h`.
* A single chat (room) is used via `src/ui/components/chat/Chat.qml` and gets its messages via `src/chat/ChatModel.h`.


## Contacts and calendars

GOnnect holds an own internal `src/contacts/AddressBook.h` filled with `src/contacts/Contact.h`.

These are filled by different feeders found in subfolders of `src/contacts`, such as `src/contacts/ldap/LDAPAddressBookFeeder.h`.

It also reads calendar infos, managed by `src/calendar/DateEventFeederManager.h` resp. `src/calendar/DateEventManager.cpp`. 

The single calendar feeders are located in subfolders beneath `src/calendar/`, such as `src/calendar/caldav/CalDAVEventFeeder.cpp`.


## Programming languages and frameworks

The main app is programmed with Qt ≥ 6.10 with C++ 23 and QML.

It uses the [PJSIP](https://github.com/pjsip/pjproject) framework and its [doc](https://docs.pjsip.org/en/latest/) for SIP functionality.


## Building

GOnnect can be build with two different build systems:

* Conan v2 and CMake (use distrobox)
* Flatpak builder

There is a Conan sub command (export-dependencies) which can be installed using:

```bash
conan config install resources/conan
```

After "export-dependencies" is exposed, all the dependencies that are not provided by the conan-center-index
can be exported using (needs to be done after something changes in `resources/conan`):

```bash
conan export-dependencies .
```

Building the Conan dependencies works as usual, but requires at least cppstd 20:

```bash
conan install . --build=missing -s compiler.cppstd=20 -s build_type=RelWithDebInfo
```

This also installs Conan presets that are used for doing the final build:

```bash
cmake --preset conan-relwithdebinfo .
cmake --build --preset conan-relwithdebinfo
```

## Packaging and platforms

GOnnect is served as a Flatpak for Linux and as MSIX for Microsoft Windows. Platform related code lives in `src/platform`. Flatpak
is prefered over "plain Linux" style packages and as much as possible should be handled over existing FDO portals.


## Code style and guidelines

* The language of the code, comments and doc is English.
* Code must be written in a very human-readable way, even if that means slight sacrifices in performance.
* Code must be written that it rarley requires comments to explain it, unless it is really helpful.
* Comments should only be written if it makes real sense, such as definitions of interfaces and difficult algorithm implementations.
* Comments should begin with an uppercase letter and only consist of line comments (no block comments if comment spans multiple lines).
* The C++ code uses clang format as defined in `.clang-format`, which is enforced by CI.
* C++ header and source files must be written in (upper) camel case.
* A .cpp file should correspond to a single header file with the exact same name.
* The QML code follows standard code format as invoked by the auto-format function of Qt Creator.
* Each QML file should start with `pragma ComponentBehavior: Bound`. The general import is `import base`.
* QML and C++ should be const whenever possible.
* The command for if/while/for etc. must always be enclosed in curly braces, even if only one line.


## Communication style

* Communicate in a short and concise way. State all relevant information but avoid blathering.
* Communication language can differ, but code, comments and doc must always be English.
* Avoid direct editing of code; present changes and only change code when explicitly stated by the user.
* Use ripgrep over grep, if avialable.
