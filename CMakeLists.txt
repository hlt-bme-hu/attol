cmake_minimum_required (VERSION 3.1)
project (attol)

set (CMAKE_CXX_STANDARD 11)

if(UNIX)
    add_compile_options( -Wall -Wpedantic )
elseif(WIN32)
    add_compile_options( /W4 /D_CRT_SECURE_NO_WARNINGS )
endif()

include_directories("${PROJECT_SOURCE_DIR}/inc")

file( GLOB HEADER_FILES "${PROJECT_SOURCE_DIR}/inc/attol/*.h")

add_executable(attol "${HEADER_FILES}"
    "${PROJECT_SOURCE_DIR}/inc/ArgParser.h"
    "${PROJECT_SOURCE_DIR}/src/attol.cpp")

add_executable(attol-utf8 "${HEADER_FILES}"
    "${PROJECT_SOURCE_DIR}/inc/ArgParser.h"
    "${PROJECT_SOURCE_DIR}/src/attol-utf8.cpp")
