cmake_minimum_required(VERSION 3.7)
project(viewports)

set(EX_PLATFORM 64)
set(EX_PLATFORM_NAME "x64")
set(CMAKE_C_STANDARD 11)

add_definitions(-Wall -O2)

file(GLOB_RECURSE SRC "src/*.c" "src/*.h")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

find_package(R REQUIRED)
message("R libraries: ${R_LIBRARIES}")
message("R include dirs: ${R_INCLUDE_DIR}")

add_library(viewports
        src/debug.c
        src/debug.h
        src/helpers.c
        src/helpers.h
        src/init.c
        src/slices.c
        src/slices.h
        src/mosaics.c
        src/mosaics.h
        src/prisms.c
        src/prisms.h
        src/common.c
        src/common.h)

include_directories(src ${R_INCLUDE_DIR})
target_link_libraries(viewports ${R_LIBRARIES})