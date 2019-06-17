project(ufovectors)
cmake_minimum_required(VERSION 2.8.7)

SET(EX_PLATFORM 64)
SET(EX_PLATFORM_NAME "x64")

add_definitions(-Wall -O2)
                # -Wall -g

file(GLOB_RECURSE SRC "src/*.c" "src/*.h")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

find_package(R REQUIRED)
message("R libraries: ${R_LIBRARIES}")
message("R include dirs: ${R_INCLUDE_DIR}")

add_library(ufovectors SHARED ${SRC})
include_directories(ufovectors ${R_INCLUDE_DIR})
target_link_libraries(ufovectors ${R_LIBRARIES})