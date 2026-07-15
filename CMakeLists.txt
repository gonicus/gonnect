cmake_minimum_required(VERSION 3.26)
project(GOnnect VERSION 2.4.0.1 LANGUAGES CXX)
include(cmake/CCache.cmake)
include(cmake/Workarounds.cmake)
include(cmake/Versioning.cmake)
include(cmake/Icons.cmake)
useCompilerCache()

set(APPLICATION_NAME       "${PROJECT_NAME}" )
set(APPLICATION_SHORTNAME  "${PROJECT_NAME}" )
set(APPLICATION_EXECUTABLE "gonnect" )
set(APPLICATION_VENDOR "GONICUS GmbH")
set(APPLICATION_ICON_NAME  "${APPLICATION_SHORTNAME}" )
set(APPLICATION_URL "https://www.gonnect-project.org")

set(MS_APPLICATION_IDENTIFIER "" CACHE STRING "Application ID used for OAUTH / Microsoft 365")

set(CMAKE_COMPILE_WARNING_AS_ERROR ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_UNITY_BUILD_BATCH_SIZE 10)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
set(CMAKE_CXX_VISIBILITY_PRESET_RELEASE hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN_RELEASE ON)
set(QT_NO_PRIVATE_MODULE_WARNING ON)

list(PREPEND CMAKE_MODULE_PATH "${PROJECT_BINARY_DIR}/generators" "${PROJECT_BINARY_DIR}/resources/windows")
list(PREPEND CMAKE_PREFIX_PATH "${PROJECT_BINARY_DIR}/generators")

# Flatpak build?
set(FLATPAK $ENV{FLATPAK_ID})

# Basic settings
option(CMAKE_UNITY_BUILD "Enable unity builds" OFF)
option(ENABLE_PCH "Enable precompiled headers" OFF)
option(BUILD_TESTING "Build tests" OFF)
option(BUILD_TOOLS "Build tools" OFF)
option(ENABLE_MULTI_INSTANCE "Allow multiple GOnnect instances" OFF)

# Contacts / Calendar plugins
option(ENABLE_AKONADI "Enable Akonadi plugins" OFF)
option(ENABLE_EDS "Enable EDS plugins" ON)
option(ENABLE_LDAP "Enable LDAP plugin" ON)
option(ENABLE_DAV "Enable DAV plugin" ON)
option(ENABLE_CSV "Enable CSV plugin" ON)
option(ENABLE_MSGRAPH "Enable MS Graph plugins" OFF)

set(APP_ID "de.gonicus.gonnect")
add_definitions(-DAPP_ID="${APP_ID}")
if(FLATPAK)
    add_definitions(-DQ_OS_FLATPAK)
endif()

if(ENABLE_MULTI_INSTANCE)
    add_definitions(-DMULTI_INSTANCE)
endif()

if(LINUX)
    if(NOT FLATPAK_APP_ID)
        set(FLATPAK_APP_ID "de.gonicus.gonnect")
    endif()
    string(REPLACE "." "/" FLATPAK_APP_PATH ${FLATPAK_APP_ID})
    add_definitions(-DFLATPAK_APP_ID="${FLATPAK_APP_ID}")
    add_definitions(-DFLATPAK_APP_PATH="/${FLATPAK_APP_PATH}")

    add_compile_options(-Wall -Wextra -Wpedantic -Werror -Wno-gnu-zero-variadic-macro-arguments)
    set(PLATFORM_QT_MODULES DBus)
else()
    set(ENABLE_AKONADI OFF)
    set(ENABLE_EDS OFF)
endif()
if(WIN32)
    add_definitions(-DNOMINMAX)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    option(ENABLE_CLAZY "Enable Clazy warnings" OFF)
    if(ENABLE_CLAZY)
        set(CMAKE_CXX_COMPILE_OBJECT "${CMAKE_CXX_COMPILE_OBJECT} -Xclang -load -Xclang ClazyPlugin.so -Xclang -add-plugin -Xclang clazy")
    endif()
endif()

find_package(Qt6 REQUIRED COMPONENTS Gui ${PLATFORM_QT_MODULES} Qml Quick QuickControls2 LinguistTools)
qt_standard_project_setup(REQUIRES 6.10
    I18N_SOURCE_LANGUAGE en
    I18N_TRANSLATED_LANGUAGES de es it fa fr ru uk
)

# Adjust target path for qm files
file(GLOB tsfiles "i18n/*.ts")
foreach(tsfile ${tsfiles})
    set_source_files_properties(${tsfile}
        PROPERTIES OUTPUT_LOCATION "${CMAKE_CURRENT_BINARY_DIR}")
endforeach()

# Versioning setup
configure_file(appversion.cpp.in appversion.cpp @ONLY)
add_library(GOnnectVersion STATIC
    ${CMAKE_CURRENT_BINARY_DIR}/appversion.cpp
)

target_include_directories(GOnnectVersion PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

if(BUILD_TOOLS)
    add_subdirectory(emoji)
endif()

configure_file(
    ${PROJECT_SOURCE_DIR}/resources/templates/templates.qrc.in
    ${PROJECT_BINARY_DIR}/resources/templates/templates.qrc @ONLY
)

add_subdirectory(src)
add_subdirectory(external/hid-rp)

if(BUILD_TESTING)
    add_subdirectory(tests)
    add_definitions(-DAPP_TESTS)
endif()

if(LINUX)
    file(READ "resources/flatpak/de.gonicus.gonnect.releases.xml" RELEASES)
    string(REGEX REPLACE "^[^<]*<\\?xml[^>]*>[ \t\r\n]*" "" RELEASES "${RELEASES}")
    configure_file(resources/flatpak/de.gonicus.gonnect.metainfo.xml.in resources/flatpak/de.gonicus.gonnect.metainfo.xml @ONLY)

    install(FILES resources/flatpak/de.gonicus.gonnect.svg DESTINATION share/icons/hicolor/scalable/apps)
    install(FILES resources/flatpak/de.gonicus.gonnect.desktop DESTINATION share/applications)
    install(FILES resources/flatpak/de.gonicus.gonnect.krunner.desktop DESTINATION share/krunner/dbusplugins)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/resources/flatpak/de.gonicus.gonnect.metainfo.xml DESTINATION share/metainfo)
    install(FILES resources/flatpak/de.gonicus.gonnect.service DESTINATION share/dbus-1/services)
    install(FILES resources/flatpak/de.gonicus.gonnect.search-provider.ini DESTINATION share/gnome-shell/search-providers)
endif()

if(PROJECT_IS_TOP_LEVEL)
    add_subdirectory(packaging)
endif()
