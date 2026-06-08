cmake_minimum_required(VERSION 3.14)

project(sqlite LANGUAGES C)

find_package(OpenSSL REQUIRED)

add_library(sqlite STATIC
    sqlite3.c
    sqlite3.h
)

# Hide the symbols from this static library so Qt cannot see them
set_target_properties(sqlite PROPERTIES
    C_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

target_compile_definitions(sqlite PUBLIC
    SQLITE_HAS_CODEC
    SQLITE_TEMP_STORE=2
    SQLITE_ENABLE_FTS5
    SQLITE_ENABLE_FTS3
    SQLITE_ENABLE_FTS4
    SQLITE_THREADSAFE=1
)

target_include_directories(sqlite PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(sqlite PUBLIC OpenSSL::Crypto m)

target_compile_options(sqlite PRIVATE "-w")
