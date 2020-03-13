cmake_minimum_required (VERSION 3.1)
project (attol)

set (CMAKE_CXX_STANDARD 11)

if(UNIX)
    add_compile_options( -Wall -Wpedantic )
elseif(WIN32)
    add_compile_options( /W4 )
endif()

include_directories("${PROJECT_SOURCE_DIR}/inc")

file( GLOB SOURCES "${PROJECT_SOURCE_DIR}/inc/*.h"
                   "${PROJECT_SOURCE_DIR}/inc/attol/*.h"
                   "${PROJECT_SOURCE_DIR}/src/*.cpp")

add_executable(attol "${SOURCES}")
