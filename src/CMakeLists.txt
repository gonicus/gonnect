find_package(Qt6 REQUIRED COMPONENTS Core Widgets Quick QuickControls2 Multimedia Sql Core5Compat Xml)
find_package(Qca-qt6 REQUIRED)

include(FindPkgConfig)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBUUID REQUIRED uuid IMPORTED_TARGET)
set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} /opt/pjsip) # Temporary add /usr/local
pkg_check_modules(PJSIP REQUIRED libpjproject)
pkg_check_modules(OPENLDAP REQUIRED IMPORTED_TARGET lber ldap)
pkg_check_modules(LIBUSB1 REQUIRED libusb-1.0 IMPORTED_TARGET)
pkg_check_modules(LIBHIDAPI REQUIRED hidapi-hidraw IMPORTED_TARGET)

add_definitions(${PJSIP_STATIC_CFLAGS} -DPJMEDIA_HAS_RTCP_XR=1 -DPJMEDIA_STREAM_ENABLE_XR=1)

set(DBUS_SOURCES)
qt_add_dbus_adaptor(DBUS_SOURCES ../specs/org.freedesktop.application.xml
    QObject
    ""
    DBusActivationAdapter
    DBusActivationAdapter
)
qt_add_dbus_adaptor(DBUS_SOURCES ../specs/org.gnome.ShellSearchProvider2.xml
    QObject
    ""
    ShellSearchProvider2Adapter
    ShellSearchProvider2Adapter
)

set_source_files_properties(
    ../specs/org.freedesktop.application.xml
    ../specs/org.mpris.MediaPlayer2.Player.xml
    ../specs/org.freedesktop.ScreenSaver.xml
    ../specs/org.freedesktop.portal.Notification.xml
    ../specs/org.freedesktop.portal.Session.xml
    ../specs/org.freedesktop.portal.Request.xml
    ../specs/org.freedesktop.portal.Secret.xml
    NO_NAMESPACE ON
)
set_source_files_properties(
    ../specs/org.freedesktop.portal.Notification.xml
    INCLUDE Notification.h
)
qt_add_dbus_interface(DBUS_SOURCES ../specs/org.freedesktop.application.xml
    DBusActivationInterface
)
qt_add_dbus_interface(DBUS_SOURCES ../specs/org.mpris.MediaPlayer2.Player.xml
    MediaPlayerInterface
)
qt_add_dbus_interface(DBUS_SOURCES ../specs/org.freedesktop.ScreenSaver.xml
    ScreenSaverInterface
)
qt_add_dbus_interface(DBUS_SOURCES ../specs/org.freedesktop.portal.Notification.xml
    NotificationInterface
)
qt_add_dbus_interface(DBUS_SOURCES ../specs/org.freedesktop.portal.GlobalShortcuts.xml
    GlobalShortcuts
)
qt_add_dbus_interface(DBUS_SOURCES ../specs/org.freedesktop.portal.Session.xml
    PortalSession
)
qt_add_dbus_interface(DBUS_SOURCES ../specs/org.freedesktop.portal.Request.xml
    PortalRequest
)
qt_add_dbus_interface(DBUS_SOURCES ../specs/org.freedesktop.portal.Settings.xml
    PortalSettings
)
qt_add_dbus_interface(DBUS_SOURCES ../specs/org.freedesktop.portal.NetworkMonitor.xml
    NetworkMonitor
)
qt_add_dbus_interface(DBUS_SOURCES ../specs/org.freedesktop.portal.Secret.xml
    PortalSecret
)

set(APP_SOURCES main.cpp)
qt_add_resources(APP_SOURCES application.qrc)
qt_add_resources(APP_SOURCES ../migrate/migrationscripts.qrc)
qt_add_resources(APP_SOURCES ../resources/templates/templates.qrc)

qt_add_executable(gonnect
    ${APP_SOURCES}
    ${DBUS_SOURCES}
)

set(gonnect_qml_singletons
    ui/components/dialogs/DialogFactory.qml
)
set_source_files_properties(${gonnect_qml_singletons} PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

if(BUILD_DEPENDENCIES)
    include(ExternalProject)

    ExternalProject_Add(qtwebdav
        GIT_REPOSITORY https://github.com/PikachuHy/QtWebDAV.git
        CMAKE_ARGS -DCMAKE_PREFIX_PATH=${QT6_INSTALL_PREFIX} -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
        PATCH_COMMAND git apply "${PROJECT_SOURCE_DIR}/resources/flatpak/patches/qtwebdav-cmake.patch"
        UPDATE_DISCONNECTED TRUE
    )
    ExternalProject_Get_Property(qtwebdav INSTALL_DIR)
    target_include_directories(gonnect SYSTEM PRIVATE ${INSTALL_DIR}/include)
    target_link_directories(gonnect PRIVATE ${INSTALL_DIR}/${CMAKE_INSTALL_LIBDIR})

    ExternalProject_Add(vcard
        GIT_REPOSITORY https://github.com/ivanenko/vCard.git
        CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
        PATCH_COMMAND git apply "${PROJECT_SOURCE_DIR}/resources/flatpak/patches/vcard-cmake.patch"
        UPDATE_DISCONNECTED TRUE
    )
    ExternalProject_Get_Property(vcard INSTALL_DIR)
    target_include_directories(gonnect SYSTEM PRIVATE ${INSTALL_DIR}/include)
    target_link_directories(gonnect PRIVATE ${INSTALL_DIR}/${CMAKE_INSTALL_LIBDIR})

    add_dependencies(gonnect vcard qtwebdav)
else()
    find_package(QtWebDAV REQUIRED)
    find_package(vCard REQUIRED)
endif()


qt_add_qml_module(gonnect
    URI base
    VERSION 1.0
    QML_FILES
        Main.qml

        ${gonnect_qml_singletons}

        ui/components/controls/AudioLevelButton.qml
        ui/components/controls/Button.qml
        ui/components/controls/CheckBox.qml
        ui/components/controls/ClipboardButton.qml
        ui/components/controls/DtmfDialer.qml
        ui/components/controls/FavIcon.qml
        ui/components/controls/HeaderIconButton.qml
        ui/components/controls/Label.qml
        ui/components/controls/MenuContactInfo.qml
        ui/components/controls/SearchDial.qml
        ui/components/controls/Switch.qml

        ui/components/dialogs/CredentialsDialog.qml
        ui/components/dialogs/BaseDialog.qml
        ui/components/dialogs/ConfirmDialog.qml
        ui/components/dialogs/InfoDialog.qml
        ui/components/dialogs/JitsiUpgradeDialog.qml

        ui/components/popups/HistoryListContextMenu.qml
        ui/components/popups/SearchResultPopup.qml

        ui/components/templatefields/TemplateFieldText.qml
        ui/components/templatefields/TemplateFieldSecret.qml
        ui/components/templatefields/TemplateFieldFile.qml

        ui/components/AvatarImage.qml
        ui/components/Badge.qml
        ui/components/BottomStatusBar.qml
        ui/components/BuddyStatusIndicator.qml
        ui/components/CallDetails.qml
        ui/components/CallItem.qml
        ui/components/CallList.qml
        ui/components/CardList.qml
        ui/components/CompleteHistoryWindow.qml
        ui/components/FavoritesList.qml
        ui/components/HistoryList.qml
        ui/components/SearchBox.qml
        ui/components/SettingsPage.qml
        ui/components/VerticalLevelMeter.qml
        ui/components/WindowHeader.qml

        ui/AboutWindow.qml
        ui/AudioEnvWindow.qml
        ui/BaseWindow.qml
        ui/CallsWindow.qml
        ui/DialWindow.qml
        ui/EmergencyCallIncomingWindow.qml
        ui/PreferredIdentityEditWindow.qml
        ui/ShortcutsWindow.qml
        ui/SipTemplateWizard.qml

    DEPENDENCIES
        QtQml.Models
    SOURCES
        Application.cpp
        Application.h
        ErrorBus.h
        ErrorBus.cpp
        StateManager.cpp
        StateManager.h
        AppSettings.h
        ReadOnlyConfdSettings.cpp
        ReadOnlyConfdSettings.h
        dbus/ExternalMediaManager.cpp
        dbus/ExternalMediaManager.h
        dbus/portal/AbstractPortal.h
        dbus/portal/AbstractPortal.cpp
        dbus/portal/AccountPortal.h
        dbus/portal/AccountPortal.cpp
        dbus/portal/BackgroundPortal.h
        dbus/portal/BackgroundPortal.cpp
        dbus/portal/GlobalShortcutPortal.h
        dbus/portal/GlobalShortcutPortal.cpp
        dbus/portal/InhibitPortal.h
        dbus/portal/InhibitPortal.cpp
        dbus/portal/SettingsPortal.h
        dbus/portal/SettingsPortal.cpp
        dbus/portal/SecretPortal.h
        dbus/portal/SecretPortal.cpp
        dbus/portal/NetworkHelper.h
        dbus/portal/NetworkHelper.cpp
        dbus/KRunnerAdapter.h
        dbus/KRunnerAdapter.cpp
        dbus/Notification.h
        dbus/Notification.cpp
        dbus/NotificationIcon.h
        dbus/NotificationIcon.cpp
        dbus/NotificationManager.h
        dbus/NotificationManager.cpp
        dbus/SearchProvider.h
        dbus/SearchProvider.cpp
        sip/CallHistory.h
        sip/CallHistory.cpp
        sip/CallHistoryItem.h
        sip/CallHistoryItem.cpp
        sip/DtmfGenerator.h
        sip/DtmfGenerator.cpp
        sip/IMHandler.h
        sip/IMHandler.cpp
        sip/KeychainSettings.h
        sip/NumberStats.h
        sip/NumberStats.cpp
        sip/SIPAccount.h
        sip/SIPAccount.cpp
        sip/SIPAccountManager.h
        sip/SIPAccountManager.cpp
        sip/SIPAudioDevice.h
        sip/SIPAudioDevice.cpp
        sip/SIPAudioManager.h
        sip/SIPAudioManager.cpp
        sip/SIPBuddy.h
        sip/SIPBuddy.cpp
        sip/SIPCall.h
        sip/SIPCall.cpp
        sip/SIPCallManager.h
        sip/SIPCallManager.cpp
        sip/SIPEventLoop.h
        sip/SIPEventLoop.cpp
        sip/SIPManager.h
        sip/SIPManager.cpp
        sip/SIPMediaConfig.h
        sip/SIPMediaConfig.cpp
        sip/SIPTemplate.h
        sip/SIPTemplate.cpp
        sip/SIPTemplateField.h
        sip/SIPTemplateField.cpp
        sip/SIPTemplates.h
        sip/SIPTemplates.cpp
        sip/SIPTemplateController.h
        sip/SIPTemplateController.cpp
        sip/SIPUserAgentConfig.h
        sip/SIPUserAgentConfig.cpp
        sip/PreferredIdentity.h
        sip/PreferredIdentity.cpp
        sip/Ringer.h
        sip/Ringer.cpp
        sip/RingTone.h
        sip/RingTone.cpp
        sip/RingToneFactory.h
        sip/RingToneFactory.cpp
        sip/Toggler.h
        sip/Toggler.cpp
        sip/TogglerManager.h
        sip/TogglerManager.cpp
        contacts/AddressBook.cpp
        contacts/AddressBook.h
        contacts/AddressBookManager.cpp
        contacts/AddressBookManager.h
        contacts/AvatarManager.cpp
        contacts/AvatarManager.h
        contacts/CardDAVAddressBookFeeder.h
        contacts/CardDAVAddressBookFeeder.cpp
        contacts/Contact.cpp
        contacts/Contact.h
        contacts/ContactSerializer.cpp
        contacts/ContactSerializer.h
        contacts/FuzzyCompare.cpp
        contacts/FuzzyCompare.h
        contacts/IAddressBookFeeder.h
        contacts/LDAPAddressBookFeeder.cpp
        contacts/LDAPAddressBookFeeder.h
        contacts/PhoneCodeLookup.h
        contacts/PhoneCodeLookup.cpp
        contacts/PhoneNumberUtil.cpp
        contacts/PhoneNumberUtil.h
        contacts/CsvFileAddressBookFeeder.h
        contacts/CsvFileAddressBookFeeder.cpp
        media/AudioPort.h
        media/AudioPort.cpp
        media/Sniffer.h
        media/Sniffer.cpp
        ui/CallsModel.h
        ui/CallsModel.cpp
        ui/CallsProxyModel.h
        ui/CallsProxyModel.cpp
        ui/EnumTranslation.h
        ui/EnumTranslation.cpp
        ui/FavoritesModel.h
        ui/FavoritesModel.cpp
        ui/HistoryModel.h
        ui/HistoryModel.cpp
        ui/HistoryProxyModel.h
        ui/HistoryProxyModel.cpp
        ui/Icons.h
        ui/Icons.cpp
        ui/MostCalledModel.h
        ui/MostCalledModel.cpp
        ui/NumberStatsModel.h
        ui/NumberStatsModel.cpp
        ui/NumberStatsProxyModel.h
        ui/NumberStatsProxyModel.cpp
        ui/SearchListModel.h
        ui/SearchListModel.cpp
        ui/SIPTemplateModel.h
        ui/SIPTemplateModel.cpp
        ui/SIPTemplatesModel.h
        ui/SIPTemplatesModel.cpp
        ui/SystemTrayMenu.h
        ui/SystemTrayMenu.cpp
        ui/Theme.h
        ui/Theme.cpp
        ui/TogglerModel.h
        ui/TogglerModel.cpp
        ui/TogglerProxyModel.h
        ui/TogglerProxyModel.cpp
        ui/ViewHelper.h
        ui/ViewHelper.cpp
        usb/IHeadsetDevice.h
        usb/HeadsetDevice.cpp
        usb/HeadsetDevice.h
        usb/HeadsetDeviceProxy.cpp
        usb/HeadsetDeviceProxy.h
        usb/HeadsetDevices.cpp
        usb/HeadsetDevices.h
        usb/ReportDescriptorEnums.h
        usb/ReportDescriptorEnums.cpp
        usb/ReportDescriptorParser.h
        usb/ReportDescriptorParser.cpp
        usb/ReportDescriptorStructs.h
        usb/ReportDescriptorStructs.cpp
)

set_target_properties(gonnect PROPERTIES
    #QT_QMLCACHEGEN_DIRECT_CALLS ON
    QT_QMLCACHEGEN_ARGUMENTS "--verbose"
)

qt_extract_metatypes(gonnect)
qt_add_translations(gonnect
    TS_FILE_BASE gonnect
    TS_FILE_DIR ${PROJECT_SOURCE_DIR}/i18n
    LUPDATE_OPTIONS -no-obsolete)

target_link_directories(gonnect PRIVATE
    ${PJSIP_STATIC_LIBRARY_DIRS}
)

target_link_libraries(gonnect
    PRIVATE
        PkgConfig::OPENLDAP
        Qt6::Core
        Qt6::CorePrivate
        Qt6::Gui
        Qt6::GuiPrivate
        Qt6::Widgets
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickControls2
        Qt6::DBus
        Qt6::Multimedia
        Qt6::Sql
        Qt6::Core5Compat
        Qt6::Xml
        ${PJSIP_STATIC_LIBRARIES}
        PkgConfig::LIBUUID
        PkgConfig::LIBUSB1
        PkgConfig::LIBHIDAPI
        GOnnectVersion
        qca-qt6
        hid-rp
        vCard
        QtWebDAV
)

target_include_directories(gonnect
    PRIVATE
    dbus
    dbus/portal
    sip
    contacts
    ui
    usb
    media
    ${PJSIP_STATIC_INCLUDE_DIRS}
    ${CMAKE_CURRENT_BINARY_DIR}
)

# Add hid-rp header only stuff as "isystem" so that we have a chance
# to ignore the warnings their code issues.
target_include_directories(gonnect
    SYSTEM PRIVATE ../external/hid-rp/hid-rp
)

install(TARGETS gonnect
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
