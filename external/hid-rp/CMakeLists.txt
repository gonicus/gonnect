cmake_minimum_required(VERSION 3.14)

project(hid-rp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

include(GNUInstallDirs)

set(INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}")

# add HID usage tables
include(ExternalProject)

set(HID_USAGE_TABLES_URL "https://github.com/IntergatedCircuits/hid-usage-tables.git" CACHE STRING "HID Usage Tables repository")
set(HID_USAGE_TABLES_REV "da4accb23f32500eebf684a0c5c5c36205cb000c" CACHE STRING "HID Usage Tables commit hash")

set(USAGE_TABLES_OUTPUT_DIR ${INCLUDES}/hid/page)

find_package(Python 3 REQUIRED)
if(Python_VERSION_MAJOR LESS 3)
    message(FATAL_ERROR "Python 3 is required for usage tables code generation.")
endif()

#ExternalProject_Add(hid-usage-tables
#    PREFIX ${CMAKE_BINARY_DIR}/hid-usage-tables
#    GIT_REPOSITORY ${HID_USAGE_TABLES_URL}
#    GIT_TAG ${HID_USAGE_TABLES_REV}
#    CONFIGURE_COMMAND ${CMAKE_COMMAND} -E chdir <SOURCE_DIR> ${Python_EXECUTABLE} <SOURCE_DIR>/src/hid/codegen/cpp.py ${USAGE_TABLES_OUTPUT_DIR}
#    BUILD_COMMAND ""
#    INSTALL_COMMAND ""
#    UPDATE_DISCONNECTED TRUE
#)

# the library itself
add_library(${PROJECT_NAME} INTERFACE)
#add_dependencies(${PROJECT_NAME} hid-usage-tables)

target_include_directories(${PROJECT_NAME}
    INTERFACE
        $<BUILD_INTERFACE:${INCLUDES}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# generate a source file and build it to verify syntax
file(GLOB_RECURSE HEADER_FILES "${INCLUDES}/*.hpp")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/compilation_test.cpp" "")
foreach(HEADER_FILE ${HEADER_FILES})
    file(RELATIVE_PATH FILE_RELATIVE_PATH
        ${INCLUDES} ${HEADER_FILE}
    )
    file(APPEND "${CMAKE_CURRENT_BINARY_DIR}/compilation_test.cpp"
        "#include \"${FILE_RELATIVE_PATH}\"\n"
    )
endforeach()
add_library(${PROJECT_NAME}-verify EXCLUDE_FROM_ALL
    ${CMAKE_CURRENT_BINARY_DIR}/compilation_test.cpp
)
target_link_libraries(${PROJECT_NAME}-verify ${PROJECT_NAME})
