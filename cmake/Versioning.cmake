find_package(Git QUIET)

# Git setup
if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")

    # Check for current tag
    execute_process(COMMAND ${GIT_EXECUTABLE} tag -l --points-at HEAD
                    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
                    RESULT_VARIABLE GIT_RESULT
                    OUTPUT_STRIP_TRAILING_WHITESPACE
                    OUTPUT_VARIABLE VERSION)
    if(NOT GIT_RESULT EQUAL "0")
        message("unable to retrieve project version from git")
    endif()

    # If there's a version - take it
    if(NOT VERSION STREQUAL "")
        string(REGEX MATCH "v([0-9a-z.+-]*)" _ ${VERSION})
        set(VERSION "${CMAKE_MATCH_1}")
    endif()
endif()

# Fall back to project version
if(VERSION STREQUAL "")
    set(VERSION ${GOnnect_VERSION})
endif()

# MSIX requires a version number with four parts
if(WIN32)
    string(REGEX MATCH "^([0-9]+)\\.([0-9]+)\\.([0-9]+)" _ ${VERSION})
    if(CMAKE_MATCH_1)
        set(MSIX_VERSION "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3}.0")
    else()
        set(MSIX_VERSION "0.0.0.0")
    endif()

    configure_file(
        ${PROJECT_SOURCE_DIR}/resources/windows/msix/AppxManifest.xml.in
        ${PROJECT_BINARY_DIR}/AppxManifest.xml
        @ONLY
    )
endif()

message("Setting GOnnect build version to ${VERSION}")
