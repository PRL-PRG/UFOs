cmake_minimum_required(VERSION 3.7)
project(ufos)

SET(EX_PLATFORM 64)
SET(EX_PLATFORM_NAME "x64")

add_definitions(-Wall -O2)
                #-Wall -g

set(CMAKE_CXX_STANDARD 11)

#file(GLOB_RECURSE SRC "src/*.c" "src/*.h")

set(CORE_SRC    src/userfaultCore.h
                src/sparseList.h
                src/sparseList.c
        src/userfaultCore.c
                src/userfaultCoreInternal.h
                src/userfaultGettersSetters.c
                src/errors.h
                src/math.h
                src/vars.h)

set(R_SRC       src/ufos.h
                src/init.c src/ufos.c)

set(TESTING_SRC testing/sparseListTest.c

                testing/userfaultSpeed.c)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

find_package(R REQUIRED)
message("R libraries: ${R_LIBRARIES}")
message("R include dirs: ${R_INCLUDE_DIR}")

find_package (Threads)

add_library(ufos SHARED ${CORE_SRC} ${R_SRC})
include_directories(ufos ${R_INCLUDE_DIR})
target_link_libraries(ufos ${R_LIBRARIES})

add_executable(ufoTest ${CORE_SRC} testing/ufoTest.c)
target_link_libraries(ufoTest ${CMAKE_THREAD_LIBS_INIT})

add_executable(strideTest ${CORE_SRC} testing/strideTest.c)
target_link_libraries(strideTest ${CMAKE_THREAD_LIBS_INIT})

add_executable(sparseListTest ${CORE_SRC} testing/sparseListTest.c)
target_link_libraries(sparseListTest ${CMAKE_THREAD_LIBS_INIT})

add_executable(userfaultSpeed ${CORE_SRC} testing/userfaultSpeed.c)
target_link_libraries(userfaultSpeed ${CMAKE_THREAD_LIBS_INIT})

