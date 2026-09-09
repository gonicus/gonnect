cmake_minimum_required(VERSION 3.20)
project(snowballstem LANGUAGES C)

include(GNUInstallDirs)

if(NOT CMAKE_C_STANDARD)
    set(CMAKE_C_STANDARD 11)
    set(CMAKE_C_STANDARD_REQUIRED ON)
endif()

set(SNOWBALL_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/src")

if(NOT SNOWBALL_GEN_DIR)
    message(FATAL_ERROR "SNOWBALL_GEN_DIR is not set")
endif()
include("${SNOWBALL_GEN_DIR}/stemmers.cmake")

file(GLOB SNOWBALL_COMPILER_SOURCES "${SNOWBALL_ROOT}/compiler/*.c")
add_executable(snowball ${SNOWBALL_COMPILER_SOURCES})
if(MSVC)
    target_compile_definitions(snowball PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()
set(SNOWBALL_EXE "$<TARGET_FILE:snowball>")
set(SNOWBALL_EXE_DEP snowball)

set(SNOWBALL_SRC_C "${CMAKE_CURRENT_BINARY_DIR}/src_c")
file(MAKE_DIRECTORY "${SNOWBALL_SRC_C}")

set(SNOWBALL_STEM_SOURCES "")
foreach(stemmer IN LISTS SNOWBALL_STEMMERS)
    # Each entry is "<algorithm>:<encoding>", e.g. "german:UTF_8".
    string(REPLACE ":" ";" stemmer_parts "${stemmer}")
    list(GET stemmer_parts 0 algorithm)
    list(GET stemmer_parts 1 encoding)

    set(charset_arg "")
    set(utf8_arg "")
    if(encoding STREQUAL "UTF_8")
        set(utf8_arg "-u")
    elseif(encoding STREQUAL "ISO_8859_2")
        set(charset_arg "${SNOWBALL_ROOT}/charsets/ISO-8859-2.sbl")
    elseif(encoding STREQUAL "KOI8_R")
        set(charset_arg "${SNOWBALL_ROOT}/charsets/KOI8-R.sbl")
    elseif(NOT encoding STREQUAL "ISO_8859_1")
        message(FATAL_ERROR "Unhandled snowball encoding: ${encoding}")
    endif()

    set(stem_base "${SNOWBALL_SRC_C}/stem_${encoding}_${algorithm}")
    add_custom_command(
        OUTPUT "${stem_base}.c" "${stem_base}.h"
        COMMAND "${SNOWBALL_EXE}" ${charset_arg} "${SNOWBALL_ROOT}/algorithms/${algorithm}.sbl"
                -o "${stem_base}"
                -eprefix "${algorithm}_${encoding}_"
                -r "${SNOWBALL_ROOT}/runtime" ${utf8_arg}
        DEPENDS "${SNOWBALL_ROOT}/algorithms/${algorithm}.sbl" ${SNOWBALL_EXE_DEP}
        COMMENT "Generating stem_${encoding}_${algorithm}.c"
        VERBATIM)
    list(APPEND SNOWBALL_STEM_SOURCES "${stem_base}.c")
endforeach()

add_library(stemmer
    ${SNOWBALL_STEM_SOURCES}
    "${SNOWBALL_ROOT}/runtime/api.c"
    "${SNOWBALL_ROOT}/runtime/utilities.c"
    "${SNOWBALL_GEN_DIR}/libstemmer.c")

target_include_directories(stemmer PRIVATE
    "${SNOWBALL_ROOT}/include"
    "${SNOWBALL_ROOT}/runtime"
    "${SNOWBALL_SRC_C}"
    "${SNOWBALL_GEN_DIR}")

set_target_properties(stemmer PROPERTIES
    OUTPUT_NAME stemmer
    WINDOWS_EXPORT_ALL_SYMBOLS ON)

if(MSVC)
    target_compile_definitions(stemmer PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

install(TARGETS stemmer
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
install(FILES "${SNOWBALL_ROOT}/include/libstemmer.h"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

if(TARGET snowball)
    install(TARGETS snowball RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
endif()
