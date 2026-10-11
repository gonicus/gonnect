if(NOT LINUX OR FLATPAK)
    message(FATAL_ERROR "Native packaging requires a non-Flatpak Linux build")
endif()
if(NOT CMAKE_INSTALL_PREFIX STREQUAL "/usr")
    message(FATAL_ERROR "Native packaging requires CMAKE_INSTALL_PREFIX=/usr")
endif()

set(CPACK_PACKAGE_NAME "gonnect")
set(CPACK_PACKAGE_CONTACT "GONICUS GmbH <info@gonicus.de>")
set(CPACK_PACKAGE_HOMEPAGE_URL "${APPLICATION_URL}")
set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")
set(CPACK_PACKAGE_RELOCATABLE FALSE)
set(CPACK_DEBIAN_PACKAGE_NAME "gonnect")
set(CPACK_DEBIAN_PACKAGE_SECTION "net")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_DEBIAN_PACKAGE_RELEASE "0native1")
set(CPACK_RPM_PACKAGE_NAME "gonnect")
set(CPACK_RPM_PACKAGE_LICENSE "GPL-2.0-or-later")
set(CPACK_RPM_PACKAGE_GROUP "Applications/Internet")
set(CPACK_RPM_PACKAGE_AUTOREQPROV ON)
set(CPACK_RPM_FILE_NAME RPM-DEFAULT)
set(CPACK_RPM_PACKAGE_RELEASE "0.native1")
set(CPACK_RPM_PACKAGE_RELEASE_DIST ON)
set(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION
    /usr/share/applications /usr/share/icons /usr/share/icons/hicolor
    /usr/share/icons/hicolor/scalable /usr/share/icons/hicolor/scalable/apps
    /usr/share/metainfo /usr/share/dbus-1 /usr/share/dbus-1/services
    /usr/share/gnome-shell /usr/share/gnome-shell/search-providers
    /usr/share/krunner /usr/share/krunner/dbusplugins /usr/share/licenses)
install(FILES "${PROJECT_SOURCE_DIR}/LICENSE" DESTINATION share/licenses/gonnect)
install(FILES "${PROJECT_SOURCE_DIR}/docs/modules/ROOT/examples/sample.conf"
    DESTINATION share/gonnect/examples)

if(EXISTS "${PROJECT_BINARY_DIR}/native-licenses")
    install(DIRECTORY "${PROJECT_BINARY_DIR}/native-licenses/"
        DESTINATION share/licenses/gonnect/third-party)
endif()

if(TARGET gonnect)
    # Bundled static OpenLDAP/OpenSSL must not interpose on system library symbols.
    target_compile_options(gonnect PRIVATE -fPIE -fstack-protector-strong -D_FORTIFY_SOURCE=3)
    target_link_options(gonnect PRIVATE -pie "LINKER:--exclude-libs,ALL" "LINKER:-z,relro,-z,now")
endif()
