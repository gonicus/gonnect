cmake_minimum_required(VERSION 3.15)
project(test_package LANGUAGES CXX)

find_package(vCard REQUIRED)

add_executable(test_package test_package.cpp)
target_link_libraries(test_package PRIVATE vcard::vcard)

set(_custom_vars vCard_FOUND)
foreach(_custom_var ${_custom_vars})
    if(DEFINED ${_custom_var})
        message(STATUS "${_custom_var}: ${${_custom_var}}")
    else()
        message(FATAL_ERROR "${_custom_var} not defined")
    endif()
endforeach()
