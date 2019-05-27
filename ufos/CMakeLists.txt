cmake_minimum_required(VERSION 3.7)
project(ufos)

SET( EX_PLATFORM 64 )
SET( EX_PLATFORM_NAME "x64" )

add_definitions(
        -Wall -O2
        # -Wall -g
)

set(CMAKE_CXX_STANDARD 11)

#file(GLOB_RECURSE SRC "src/*.c" "src/*.h")

set(SRC src/init.c
        include/ufos.h
        src/ufos.c
        src/bTree.h
        src/bTree.c
        src/sparseList.h
        src/sparseList.c
        src/stride.c
        include/userfaultCore.h
        src/userfaultCore.c
        src/userFaultCoreInternal.h
        src/userfaultGettersSetters.c)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
#
find_package(R REQUIRED)
message("R libraries: ${R_LIBRARIES}")
message("R include dirs: ${R_INCLUDE_DIR}")
#
add_library(ufos SHARED ${SRC})
#
include_directories(ufos ${R_INCLUDE_DIR})
target_link_libraries(ufos ${R_LIBRARIES})