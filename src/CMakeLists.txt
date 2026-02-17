find_package(Qt6 REQUIRED COMPONENTS
    Core
    CorePrivate
    GuiPrivate
    Concurrent
    Core5Compat
    Quick
    QuickControls2
    Multimedia
    Sql
    Network
    NetworkAuth
    Widgets
    WebEngineCore
    WebEngineQuick
    Xml)

find_package(Qca-qt6 REQUIRED)
find_package(logfault CONFIG REQUIRED)

if(UNIX)
    include(FindPkgConfig)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(PJSIP REQUIRED libpjproject)
    pkg_check_modules(LIBUUID REQUIRED uuid IMPORTED_TARGET)
    pkg_check_modules(LIBUSB1 REQUIRED libusb-1.0 IMPORTED_TARGET)
    pkg_check_modules(LIBHIDAPI REQUIRED hidapi-hidraw IMPORTED_TARGET)
    list(APPEND PLATFORM_LIBRARIES
        ${PJSIP_STATIC_LIBRARIES}
        PkgConfig::LIBUUID
        PkgConfig::LIBUSB1
        PkgConfig::LIBHIDAPI)

    add_definitions(${PJSIP_STATIC_CFLAGS})
    include_directories(${PJSIP_STATIC_INCLUDE_DIRS})
    link_directories(${PJSIP_STATIC_LIBRARY_DIRS})
else()
    find_package(pjproject REQUIRED)
    find_package(hidapi REQUIRED)
    find_package(libusb REQUIRED)
    find_package(wintoast REQUIRED)

    list(APPEND PLATFORM_LIBRARIES
        pjproject::pjproject
        hidapi::hidapi
        libusb::libusb
        wintoast::wintoast)
endif()

add_definitions(-DPJMEDIA_HAS_RTCP_XR=1 -DPJMEDIA_STREAM_ENABLE_XR=1)

set(APP_SOURCES main.cpp)
set(APP_OUTPUT_NAME gonnect)

if(APPLE)
    set(APP_OUTPUT_NAME GOnnect)
    set(app_icon_macos "${PROJECT_SOURCE_DIR}/resources/artwork/gonnect.icns")
    set_source_files_properties(${app_icon_macos} PROPERTIES
           MACOSX_PACKAGE_LOCATION "Resources")

    list(APPEND PLATFORM_LIBRARIES
        "-framework Security"
        "-framework Network")

elseif(LINUX)
    pkg_check_modules(LIBNOTIFY REQUIRED libnotify IMPORTED_TARGET)
    pkg_check_modules(LIBPULSE REQUIRED libpulse IMPORTED_TARGET)

    list(APPEND PLATFORM_LIBRARIES
        PkgConfig::LIBPULSE
        PkgConfig::LIBNOTIFY
        Qt6::DBus)

    set(DBUS_SOURCES)
    qt_add_dbus_adaptor(DBUS_SOURCES ../specs/org.freedesktop.application.xml
        QObject
        ""
        DBusActivationAdapter
        DBusActivationAdapter
    )
    qt_add_dbus_adaptor(DBUS_SOURCES ../specs/de.gonicus.gonnect.api.xml
        QObject
        ""
        GOnnectDBusAPIAdaptor
        GOnnectDBusAPIAdaptor
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
        ../specs/org.freedesktop.portal.Session.xml
        ../specs/org.freedesktop.portal.Request.xml
        ../specs/org.freedesktop.portal.Secret.xml
        NO_NAMESPACE ON
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
    qt_add_dbus_interface(DBUS_SOURCES ../specs/org.freedesktop.portal.GlobalShortcuts.xml
        GlobalShortcutsInterface
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

elseif(WIN32)
    list(APPEND PLATFORM_LIBRARIES Iphlpapi.lib)
    list(APPEND APP_SOURCES gonnect.rc)
endif()

qt_add_big_resources(APP_SOURCES application.qrc)
qt_add_big_resources(APP_SOURCES ${CMAKE_CURRENT_BINARY_DIR}/icons.qrc)
qt_add_resources(APP_SOURCES ../migrate/migrationscripts.qrc)
qt_add_resources(APP_SOURCES ../resources/templates/templates.qrc)

qt_add_executable(gonnect
    ${app_icon_macos}
    ${APP_SOURCES}
    ${DBUS_SOURCES}
)

set_target_properties(gonnect PROPERTIES
    QT_QMLCACHEGEN_ARGUMENTS "--only-bytecode"
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_ICON_FILE gonnect.icns
    MACOSX_BUNDLE_INFO_PLIST "${PROJECT_SOURCE_DIR}/resources/macos/Info.plist"
    OUTPUT_NAME ${APP_OUTPUT_NAME}
)

set(gonnect_qml_singletons
    ui/components/dialogs/DialogFactory.qml
)
set_source_files_properties(${gonnect_qml_singletons} PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

add_subdirectory(contacts/ldap)
add_subdirectory(contacts/csv)
add_subdirectory(calendar/caldav)
add_subdirectory(contacts/carddav)
add_subdirectory(contacts/eds)
add_subdirectory(calendar/eds)
add_subdirectory(contacts/akonadi)
add_subdirectory(calendar/akonadi)
add_subdirectory(contacts/msgraph)

find_package(Qt6Keychain REQUIRED)

set_source_files_properties(ui/scripts/util.js PROPERTIES QT_QML_SOURCE_TYPENAME Util)

if(FLATPAK)
    set(PLATFORM_SOURCES
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

        dbus/GOnnectDBusAPI.h
        dbus/GOnnectDBusAPI.cpp
        dbus/KRunnerAdapter.h
        dbus/KRunnerAdapter.cpp

        platform/flatpak/FlatpakUserInfo.h
        platform/flatpak/FlatpakUserInfo.cpp
        platform/linux/LinuxNotificationManager.h
        platform/linux/LinuxNotificationManager.cpp
        platform/linux/MPRISExternalMediaManager.cpp
        platform/linux/MPRISExternalMediaManager.h
        platform/flatpak/FlatpakBackgroundManager.h
        platform/flatpak/FlatpakBackgroundManager.cpp
        platform/flatpak/FlatpakThemeManager.h
        platform/flatpak/FlatpakThemeManager.cpp
        platform/flatpak/FlatpakNetworkHelper.h
        platform/flatpak/FlatpakNetworkHelper.cpp
        platform/flatpak/FlatpakInhibitHelper.h
        platform/flatpak/FlatpakInhibitHelper.cpp
        platform/flatpak/FlatpakGlobalShortcuts.h
        platform/flatpak/FlatpakGlobalShortcuts.cpp
        platform/linux/LinuxDesktopSearchProvider.h
        platform/linux/LinuxDesktopSearchProvider.cpp)

elseif(LINUX)
    set(PLATFORM_SOURCES
        dbus/GOnnectDBusAPI.h
        dbus/GOnnectDBusAPI.cpp
        dbus/KRunnerAdapter.h
        dbus/KRunnerAdapter.cpp

        platform/linux/LinuxUserInfo.h
        platform/linux/LinuxUserInfo.cpp
        platform/linux/LinuxNotificationManager.h
        platform/linux/LinuxNotificationManager.cpp
        platform/linux/LinuxNetworkHelper.cpp
        platform/linux/LinuxNetworkHelper.h
        platform/linux/MPRISExternalMediaManager.cpp
        platform/linux/MPRISExternalMediaManager.h

        platform/dummy/DummyBackgroundManager.cpp
        platform/dummy/DummyBackgroundManager.h
        platform/dummy/DummyGlobalShortcuts.h
        platform/dummy/DummyGlobalShortcuts.cpp

        platform/dummy/DummyThemeManager.cpp
        platform/dummy/DummyThemeManager.h
        platform/dummy/DummyInhibitHelper.h
        platform/dummy/DummyInhibitHelper.cpp

        platform/linux/LinuxDesktopSearchProvider.h
        platform/linux/LinuxDesktopSearchProvider.cpp)

elseif(APPLE)
    set(PLATFORM_SOURCES
        platform/dummy/DummyNotificationManager.h
        platform/dummy/DummyNotificationManager.cpp
        platform/dummy/DummyUserInfo.cpp
        platform/dummy/DummyUserInfo.h
        platform/dummy/DummyExternalMediaManager.cpp
        platform/dummy/DummyExternalMediaManager.h
        platform/dummy/DummyBackgroundManager.cpp
        platform/dummy/DummyBackgroundManager.h
        platform/dummy/DummyThemeManager.cpp
        platform/dummy/DummyThemeManager.h
        platform/macos/MacOSNetworkHelper.cpp
        platform/macos/MacOSNetworkHelper.h
        platform/dummy/DummyInhibitHelper.h
        platform/dummy/DummyInhibitHelper.cpp
        platform/dummy/DummyGlobalShortcuts.h
        platform/dummy/DummyGlobalShortcuts.cpp
        platform/dummy/DummySearchProvider.h
        platform/dummy/DummySearchProvider.cpp)
elseif(WIN32)
    set(PLATFORM_SOURCES
        platform/windows/WindowsUserInfo.cpp
        platform/windows/WindowsUserInfo.h
        platform/dummy/DummyExternalMediaManager.cpp
        platform/dummy/DummyExternalMediaManager.h
        platform/dummy/DummyBackgroundManager.cpp
        platform/dummy/DummyBackgroundManager.h
        platform/windows/WindowsThemeManager.cpp
        platform/windows/WindowsThemeManager.h
        platform/windows/EventLog.h
        platform/windows/WindowsEventLogHandler.h
        platform/windows/WindowsNetworkHelper.cpp
        platform/windows/WindowsNetworkHelper.h
        platform/windows/WindowsNotificationManager.h
        platform/windows/WindowsNotificationManager.cpp
        platform/windows/WindowsInhibitHelper.h
        platform/windows/WindowsInhibitHelper.cpp
        platform/dummy/DummyGlobalShortcuts.h
        platform/dummy/DummyGlobalShortcuts.cpp
        platform/dummy/DummySearchProvider.h
        platform/dummy/DummySearchProvider.cpp
    )
else()
    set(PLATFORM_SOURCES
        platform/dummy/DummyNotificationManager.h
        platform/dummy/DummyNotificationManager.cpp
        platform/dummy/DummyUserInfo.cpp
        platform/dummy/DummyUserInfo.h
        platform/dummy/DummyExternalMediaManager.cpp
        platform/dummy/DummyExternalMediaManager.h
        platform/dummy/DummyBackgroundManager.cpp
        platform/dummy/DummyBackgroundManager.h
        platform/dummy/DummyThemeManager.cpp
        platform/dummy/DummyThemeManager.h
        platform/dummy/DummyNetworkHelper.cpp
        platform/dummy/DummyNetworkHelper.h
        platform/dummy/DummyInhibitHelper.h
        platform/dummy/DummyInhibitHelper.cpp
        platform/dummy/DummyGlobalShortcuts.h
        platform/dummy/DummyGlobalShortcuts.cpp
        platform/dummy/DummySearchProvider.h
        platform/dummy/DummySearchProvider.cpp)
endif()

qt_add_qml_module(gonnect
    URI base
    VERSION 1.0
    QML_FILES
        Main.qml

        ${gonnect_qml_singletons}

        ui/components/chat/ChatMessageBox.qml
        ui/components/chat/ChatMessageList.qml
        ui/components/chat/ChatMessageListItem.qml
        ui/components/chat/ChatRoomList.qml
        ui/components/chat/ChatRoomListItem.qml

        ui/components/controls/AudioLevelButton.qml
        ui/components/controls/BarButton.qml
        ui/components/controls/Button.qml
        ui/components/controls/CheckBox.qml
        ui/components/controls/ClipboardButton.qml
        ui/components/controls/ControlBar.qml
        ui/components/controls/DtmfDialer.qml
        ui/components/controls/EmojiButton.qml
        ui/components/controls/EmojiPicker.qml
        ui/components/controls/FavIcon.qml
        ui/components/controls/FavoriteListItemBig.qml
        ui/components/controls/FavoriteListItemSmall.qml
        ui/components/controls/FirstAidButton.qml
        ui/components/controls/HeaderIconButton.qml
        ui/components/controls/Label.qml
        ui/components/controls/MenuContactInfo.qml
        ui/components/controls/SearchDial.qml
        ui/components/controls/SearchField.qml
        ui/components/controls/Switch.qml
        ui/components/controls/TabBar.qml
        ui/components/controls/TabButton.qml
        ui/components/controls/TextField.qml

        ui/components/dialogs/CredentialsDialog.qml
        ui/components/dialogs/BaseDialog.qml
        ui/components/dialogs/ConfirmDialog.qml
        ui/components/dialogs/InfoDialog.qml
        # Dynamic UI
        ui/components/BaseWidget.qml
        ui/components/DateEventsWidget.qml
        ui/components/FavoritesWidget.qml
        ui/components/HistoryWidget.qml
        ui/components/WebViewWidget.qml
        ui/components/AdditionalSettings.qml

        ui/components/pages/Call.qml
        ui/components/pages/Chats.qml
        ui/components/pages/Conference.qml
        # Dynamic UI
        ui/components/pages/BasePage.qml

        ui/components/popups/AudioDeviceMenu.qml
        ui/components/popups/BurgerMenu.qml
        ui/components/popups/DialInInfo.qml
        ui/components/popups/FirstAid.qml
        ui/components/popups/HistoryListContextMenu.qml
        ui/components/popups/JitsiHistoryListContextMenu.qml
        ui/components/popups/Menu.qml
        ui/components/popups/SearchResultPopup.qml
        ui/components/popups/VideoDeviceMenu.qml
        ui/components/popups/VolumePopup.qml

        ui/components/search/IdentitySelector.qml
        ui/components/search/KeyNavigator.qml
        ui/components/search/SearchCategoryItem.qml
        ui/components/search/SearchCategoryList.qml
        ui/components/search/SearchResultCategory.qml
        ui/components/search/SearchResultItem.qml
        ui/components/search/SearchResultNumberItem.qml

        ui/components/templatefields/TemplateFieldText.qml
        ui/components/templatefields/TemplateFieldFile.qml

        ui/components/AdditionalInfo.qml
        ui/components/AvatarImage.qml
        ui/components/Badge.qml
        ui/components/BottomStatusBar.qml
        ui/components/BuddyStatusIndicator.qml
        ui/components/CallButtonBar.qml
        ui/components/CallDetails.qml
        ui/components/CallerBigAvatar.qml
        ui/components/CallItem.qml
        ui/components/CallList.qml
        ui/components/CallSideBar.qml
        ui/components/Card.qml
        ui/components/CardHeading.qml
        ui/components/CardList.qml
        ui/components/ChatSideBar.qml
        ui/components/ConferenceButtonBar.qml
        ui/components/CustomWindowHeader.qml
        ui/components/DateEventsList.qml
        ui/components/FavoritesList.qml
        ui/components/HistoryList.qml
        ui/components/MainTabBar.qml
        ui/components/ParticipantsList.qml
        ui/components/SearchBox.qml
        ui/components/SettingsPage.qml
        ui/components/TogglerList.qml
        ui/components/VerticalLevelMeter.qml
        ui/components/WindowHeader.qml
        # Dynamic UI
        ui/components/EditModeOptions.qml

        ui/scripts/util.js

        ui/AboutWindow.qml
        ui/AudioEnvWindow.qml
        ui/BaseWindow.qml
        ui/GonnectWindow.qml
        ui/EmergencyCallIncomingWindow.qml
        ui/PreferredIdentityEditWindow.qml
        ui/ShortcutsWindow.qml
        ui/SipTemplateWizard.qml
        # Dynamic UI
        ui/CommonPages.qml
        ui/CommonWidgets.qml
        ui/PageModel.qml
        ui/PageReader.qml
        ui/PageCreationWindow.qml
        ui/PageWriter.qml
        ui/WidgetModel.qml
        ui/WidgetSelectionWindow.qml


    DEPENDENCIES
        QtQml.Models

    SOURCES
        Application.cpp
        Application.h
        AppSettings.h
        AuthManager.cpp
        AuthManager.h
        ErrorBus.h
        ErrorBus.cpp
        GlobalCallState.h
        GlobalCallState.cpp
        GlobalInfo.h
        GlobalInfo.cpp
        GlobalMuteState.h
        GlobalMuteState.cpp
        ICallState.h
        ICallState.cpp
        ReadOnlyConfdSettings.cpp
        ReadOnlyConfdSettings.h
        KeychainSettings.cpp
        KeychainSettings.h
        StateManager.cpp
        StateManager.h
        UISettings.cpp
        UISettings.h

        sip/CallHistory.h
        sip/CallHistory.cpp
        sip/CallHistoryItem.h
        sip/CallHistoryItem.cpp
        sip/DtmfGenerator.h
        sip/DtmfGenerator.cpp
        sip/IMHandler.h
        sip/IMHandler.cpp
        sip/NumberStat.h
        sip/NumberStats.h
        sip/NumberStats.cpp
        sip/SIPAccount.h
        sip/SIPAccount.cpp
        sip/SIPAccountManager.h
        sip/SIPAccountManager.cpp
        sip/SIPAudioDevice.h
        sip/SIPAudioDevice.cpp
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
        sip/PreferredIdentityValidator.h
        sip/PreferredIdentityValidator.cpp
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

        calendar/DateEvent.h
        calendar/DateEvent.cpp
        calendar/DateEventFeederManager.h
        calendar/DateEventFeederManager.cpp
        calendar/DateEventManager.h
        calendar/DateEventManager.cpp
        calendar/IDateEventFeeder.h
        calendar/IDateEventFeederFactory.h

        chat/IChatProvider.h
        chat/IChatProvider.cpp
        chat/IChatRoom.h
        chat/IChatRoom.cpp
        chat/ChatConnectorManager.h
        chat/ChatConnectorManager.cpp
        chat/ChatMessage.h
        chat/ChatMessage.cpp
        chat/ChatModel.h
        chat/ChatModel.cpp
        chat/ChatProxyModel.h
        chat/ChatProxyModel.cpp
        chat/ChatRoomModel.h
        chat/ChatRoomModel.cpp
        chat/ChatRoomProxyModel.h
        chat/ChatRoomProxyModel.cpp
        chat/ConferenceChatRoom.h
        chat/ConferenceChatRoom.cpp

        conference/ConferenceParticipant.h
        conference/ConferenceParticipant.cpp
        conference/IConferenceConnector.h

        contacts/AddressBook.cpp
        contacts/AddressBook.h
        contacts/AddressBookManager.cpp
        contacts/AddressBookManager.h
        contacts/AvatarManager.cpp
        contacts/AvatarManager.h
        contacts/BlockInfo.cpp
        contacts/BlockInfo.h
        contacts/Contact.cpp
        contacts/Contact.h
        contacts/ContactSerializer.cpp
        contacts/ContactSerializer.h
        contacts/EmergencyContact.h
        contacts/EmergencyContact.cpp
        contacts/EmergencyContactsModel.h
        contacts/EmergencyContactsModel.cpp
        contacts/FuzzyCompare.cpp
        contacts/FuzzyCompare.h
        contacts/IAddressBookFeeder.h
        contacts/PhoneCodeLookup.h
        contacts/PhoneCodeLookup.cpp
        contacts/PhoneNumberUtil.cpp
        contacts/PhoneNumberUtil.h
        contacts/ResponseLoader.h
        contacts/ResponseLoader.cpp
        contacts/ResponseItem.h
        contacts/ResponseItem.cpp
        contacts/ResponseTreeItem.h
        contacts/ResponseTreeItem.cpp
        contacts/ResponseTreeModel.h
        contacts/ResponseTreeModel.cpp
        contacts/IAddressBookFactory.h

        media/AudioManager.h
        media/AudioManager.cpp
        media/AudioPort.h
        media/AudioPort.cpp
        media/Sniffer.h
        media/Sniffer.cpp
        media/VideoManager.h
        media/VideoManager.cpp

        platform/Credentials.h
        platform/Credentials.cpp
        platform/UserInfo.h
        platform/Notification.h
        platform/Notification.cpp
        platform/NotificationIcon.h
        platform/NotificationIcon.cpp
        platform/NotificationManager.h
        platform/ExternalMediaManager.h
        platform/BackgroundManager.h
        platform/ThemeManager.h
        platform/ThemeManager.cpp
        platform/NetworkHelper.cpp
        platform/NetworkHelper.h
        platform/InhibitHelper.h
        platform/GlobalShortcuts.h
        platform/SearchProvider.h
        ${PLATFORM_SOURCES}

        ui/CallsModel.h
        ui/CallsModel.cpp
        ui/DateEventsModel.h
        ui/DateEventsModel.cpp
        ui/EmojiInfo.h
        ui/EmojiInfo.cpp
        ui/EmojiGroupsModel.h
        ui/EmojiGroupsModel.cpp
        ui/EmojiModel.h
        ui/EmojiModel.cpp
        ui/EmojiResolver.h
        ui/EmojiResolver.cpp
        ui/EmojiProxyModel.h
        ui/EmojiProxyModel.cpp
        ui/CallsProxyModel.h
        ui/CallsProxyModel.cpp
        ui/ContactSourceInfoModel.h
        ui/ContactSourceInfoModel.cpp
        ui/EnumTranslation.h
        ui/EnumTranslation.cpp
        ui/FavoritesModel.h
        ui/FavoritesModel.cpp
        ui/FavoritesProxyModel.h
        ui/FavoritesProxyModel.cpp
        ui/HistoryContactSearchModel.h
        ui/HistoryContactSearchModel.cpp
        ui/HistoryContactSearchProxyModel.h
        ui/HistoryContactSearchProxyModel.cpp
        ui/HistoryModel.h
        ui/HistoryModel.cpp
        ui/HistoryProxyModel.h
        ui/HistoryProxyModel.cpp
        ui/Icons.h
        ui/Icons.cpp
        ui/JitsiConnector.h
        ui/JitsiConnector.cpp
        ui/JitsiMediaDevice.h
        ui/JitsiMediaDevice.cpp
        ui/MostCalledModel.h
        ui/MostCalledModel.cpp
        ui/NumberStatsModel.h
        ui/NumberStatsModel.cpp
        ui/NumberStatsProxyModel.h
        ui/NumberStatsProxyModel.cpp
        ui/RandomRoomNameGenerator.h
        ui/RandomRoomNameGenerator.cpp
        ui/ParticipantsModel.h
        ui/ParticipantsModel.cpp
        ui/SearchListModel.h
        ui/SearchListModel.cpp
        ui/SearchListProxyModel.h
        ui/SearchListProxyModel.cpp
        ui/SimpleSortProxyModel.h
        ui/SimpleSortProxyModel.cpp
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
        usb/USBDevices.cpp
        usb/USBDevices.h
        usb/ReportDescriptorEnums.h
        usb/ReportDescriptorEnums.cpp
        usb/ReportDescriptorParser.h
        usb/ReportDescriptorParser.cpp
        usb/ReportDescriptorStructs.h
        usb/ReportDescriptorStructs.cpp

        usb/busylight/IBusylightDevice.h
        usb/busylight/IBusylightDevice.cpp
        usb/busylight/BusylightDeviceManager.h
        usb/busylight/BusylightDeviceManager.cpp
        usb/busylight/KuandoOmega.h
        usb/busylight/KuandoOmega.cpp
        usb/busylight/LuxaforFlag.h
        usb/busylight/LuxaforFlag.cpp
#        usb/busylight/LitraBeamLX.h
#        usb/busylight/LitraBeamLX.cpp
        usb/busylight/LitraGlow.h
        usb/busylight/LitraGlow.cpp
)

if(LINUX)
    set_target_properties(gonnect PROPERTIES LINK_FLAGS_RELEASE -s)
endif()

qt_extract_metatypes(gonnect)
qt_add_translations(gonnect
    TS_FILE_BASE gonnect
    TS_FILE_DIR ${PROJECT_SOURCE_DIR}/i18n
    LUPDATE_OPTIONS -no-obsolete)

if(ENABLE_PCH)
    target_precompile_headers(gonnect PRIVATE pch.h)
endif()

target_link_libraries(gonnect
    PRIVATE
        Qt6::Core
        Qt6::CorePrivate
        Qt6::Concurrent
        Qt6::Gui
        Qt6::Widgets
        Qt6::GuiPrivate
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickControls2
        Qt6::Multimedia
        Qt6::Sql
        Qt6::Network
        Qt6::NetworkAuth
        Qt6::WebEngineCore
        Qt6::WebEngineQuick
        Qt6::Core5Compat
        GOnnectVersion
        ${PLATFORM_LIBRARIES}
        qca-qt6
        qt6keychain
        hid-rp
        ${AKONADI_CONTACT_FACTORY}
        ${AKONADI_CALENDAR_FACTORY}
        ${CARDDAV_CONTACT_FACTORY}
        ${CSV_ADDRESSBOOK_FACTORY}
        ${CALDAV_CALENDAR_FACTORY}
        ${EDS_ADDRESSBOOK_LIBRARIES}
        ${EDS_CALENDAR_LIBRARIES}
        ${LDAP_ADDRESSBOOK_FACTORY}
        ${MSGRAPH_ADDRESSBOOK_FACTORY}
)

qt_import_plugins(gonnect INCLUDE
     ${AKONADI_CONTACT_FACTORY}
     ${AKONADI_CALENDAR_FACTORY}
     ${CARDDAV_CONTACT_FACTORY}
     ${CSV_ADDRESSBOOK_FACTORY}
     ${CALDAV_CALENDAR_FACTORY}
     ${MSGRAPH_ADDRESSBOOK_FACTORY}
     ${EDS_ADDRESSBOOK_FACTORY}
     ${EDS_CALENDAR_FACTORY}
     ${LDAP_ADDRESSBOOK_FACTORY})

target_include_directories(gonnect
    PRIVATE
    chat
    dbus
    dbus/portal
    sip
    calendar
    contacts
    conference
    ui
    ui/chat
    usb
    usb/busylight
    media
    platform
    ${logfault_INCLUDE_DIRS}
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_compile_definitions(gonnect
   PRIVATE
    QT_DISABLE_DEPRECATED_BEFORE=0x060900
    QT_NO_KEYWORDS)

# Add hid-rp header only stuff as "isystem" so that we have a chance
# to ignore the warnings their code issues.
target_include_directories(gonnect
    SYSTEM PRIVATE ../external/hid-rp/hid-rp
)

if(APPLE OR WIN32)
    qt_generate_deploy_qml_app_script(
        TARGET gonnect
        OUTPUT_SCRIPT deploy_script
        MACOS_BUNDLE_POST_BUILD
        NO_UNSUPPORTED_PLATFORM_ERROR
        DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM
    )
    install(SCRIPT ${deploy_script})
endif()

install(TARGETS gonnect
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

