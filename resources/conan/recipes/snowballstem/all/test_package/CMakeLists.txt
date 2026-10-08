cmake_minimum_required(VERSION 3.20)
project(test_package LANGUAGES C)

find_package(SnowballStem REQUIRED CONFIG)

add_executable(test_package test_package.c)
target_link_libraries(test_package PRIVATE SnowballStem::libstemmer)
