cmake_minimum_required(VERSION 3.13)
set(PROJ_ROOT "${CMAKE_CURRENT_LIST_DIR}/../..")
include("./../../Cmake/config.cmake")

set (LIB_NAME "Log")

set(SOURCE_FILES LogLib.cpp LogLib.h)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${BIN_OPATH}/${LIB_NAME}") # .so and .dylib
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${BIN_OPATH}/${LIB_NAME}") # .lib and .a

project("${LIB_NAME}")

add_library("${LIB_NAME}" ${SOURCE_FILES})