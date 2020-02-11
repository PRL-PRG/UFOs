project(badstrings)
cmake_minimum_required(VERSION 2.8.7)

SET(EX_PLATFORM 64)
SET(EX_PLATFORM_NAME "x64")

add_definitions(-Wall -O2)

file(GLOB_RECURSE SRC "src/*.c" "src/*.h")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

find_package(R REQUIRED)
message("R libraries: ${R_LIBRARIES}")
message("R include dirs: ${R_INCLUDE_DIR}")

add_library(badstrings SHARED ${SRC})
include_directories(badstrings ${R_INCLUDE_DIR})
target_link_libraries(badstrings ${R_LIBRARIES})
