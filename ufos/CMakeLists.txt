cmake_minimum_required(VERSION 3.7)
project(ufos)

SET(EX_PLATFORM 64)
SET(EX_PLATFORM_NAME "x64")

add_definitions(-Wall -g -D__UFOs_CoreDev__)
                #-Wall -g

set(CMAKE_CXX_STANDARD 11)

#file(GLOB_RECURSE SRC "src/*.c" "src/*.h")

set(CORE_SRC    src/mappedMemory/userfaultCore.h
                src/mappedMemory/sparseList.h
                src/mappedMemory/sparseList.c
                src/mappedMemory/userfaultCore.c
                src/mappedMemory/userFaultCoreInternal.h
                src/mappedMemory/userfaultGettersSetters.c
                src/mappedMemory/oroboros.h
                src/mappedMemory/oroboros.c)

set(UNSTDLIB_SRC
                src/unstdLib/errors.h
                src/unstdLib/math.h
                src/unstdLib/vars.h)

set(R_SRC       src/ufos.h
                src/ufos.c
                src/init.c)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

find_package(R REQUIRED)
message("R libraries: ${R_LIBRARIES}")
message("R include dirs: ${R_INCLUDE_DIR}")

find_package (Threads)

add_library(ufos SHARED ${CORE_SRC} ${R_SRC} ${UNSTDLIB_SRC})
include_directories(ufos ${R_INCLUDE_DIR})
target_link_libraries(ufos ${R_LIBRARIES})

add_executable(ufoTest ${CORE_SRC} ${UNSTDLIB_SRC} src/mappedMemory/ufoTest.c)
target_link_libraries(ufoTest ${CMAKE_THREAD_LIBS_INIT})


add_executable(oroborosTest src/mappedMemory/oroboros.c src/mappedMemory/oroboros.h src/mappedMemory/oroborosTest.c)
#target_link_libraries(oroborosTest ${CMAKE_THREAD_LIBS_INIT})

add_executable(strideTest ${CORE_SRC} src/mappedMemory/stride.c)
target_link_libraries(strideTest ${CMAKE_THREAD_LIBS_INIT})

add_executable(sparseListTest ${CORE_SRC} src/mappedMemory/sparseListTest.c)
target_link_libraries(sparseListTest ${CMAKE_THREAD_LIBS_INIT})

add_executable(userfaultSpeed ${CORE_SRC} src/mappedMemory/userfaultSpeed.c)
target_link_libraries(userfaultSpeed ${CMAKE_THREAD_LIBS_INIT})

