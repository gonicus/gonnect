cmake_minimum_required(VERSION 3.26)
project(GOnnect VERSION 2.0.7 LANGUAGES CXX)
include(cmake/CCache.cmake)
useCompilerCache()

set(CMAKE_COMPILE_WARNING_AS_ERROR ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_UNITY_BUILD_BATCH_SIZE 10)
set(QT_NO_PRIVATE_MODULE_WARNING ON)

option(BUILD_DEPENDENCIES "Build dependencies" OFF)
option(CMAKE_UNITY_BUILD "Enable unity builds" OFF)
option(ENABLE_PCH "Enable precompiled headers" OFF)

set(APP_ID "de.gonicus.gonnect")
add_definitions(-DAPP_ID="${APP_ID}")

if(LINUX)
    if(NOT FLATPAK_APP_ID)
        set(FLATPAK_APP_ID "de.gonicus.gonnect")
    endif()
    string(REPLACE "." "/" FLATPAK_APP_PATH ${FLATPAK_APP_ID})
    add_definitions(-DFLATPAK_APP_ID="${FLATPAK_APP_ID}")
    add_definitions(-DFLATPAK_APP_PATH="/${FLATPAK_APP_PATH}")

    add_compile_options(-Wall -Wextra -Wpedantic -Werror -Wno-gnu-zero-variadic-macro-arguments)
    set(PLATFORM_QT_MODULES DBus)
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
qt_standard_project_setup(REQUIRES 6.9
    I18N_SOURCE_LANGUAGE en
    I18N_TRANSLATED_LANGUAGES de es it
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

add_subdirectory(emoji)
add_subdirectory(src)
add_subdirectory(external/hid-rp)

if(${BUILD_TESTING})
    add_subdirectory(tests)
    add_definitions(-DAPP_TESTS)
endif()

if(LINUX)
    install(FILES resources/flatpak/de.gonicus.gonnect.svg DESTINATION share/icons/hicolor/scalable/apps)
    install(FILES resources/flatpak/de.gonicus.gonnect.desktop DESTINATION share/applications)
    install(FILES resources/flatpak/de.gonicus.gonnect.krunner.desktop DESTINATION share/krunner/dbusplugins)
    install(FILES resources/flatpak/de.gonicus.gonnect.metainfo.xml DESTINATION share/metainfo)
    install(FILES resources/flatpak/de.gonicus.gonnect.releases.xml DESTINATION share/metainfo/releases)
    install(FILES resources/flatpak/de.gonicus.gonnect.service DESTINATION share/dbus-1/services)
    install(FILES resources/flatpak/de.gonicus.gonnect.search-provider.ini DESTINATION share/gnome-shell/search-providers)
endif()
