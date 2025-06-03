cmake_minimum_required(VERSION 3.10)

project(libaaio)

add_library(aaio STATIC libaaio-0.3.1/aaio.c)

target_include_directories(aaio PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/libaaio-0.3.1)