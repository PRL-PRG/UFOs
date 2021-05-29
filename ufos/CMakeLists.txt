cmake_minimum_required(VERSION 3.7)
project(ufos)

SET(EX_PLATFORM 64)
SET(EX_PLATFORM_NAME "x64")

add_definitions(-Wall -O2 -g -D__UFOs_CoreDev__ -DMAKE_SURE -DNDEBUG)

enable_language( C CXX ASM )
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

set(BLAKE_SRC
        src/mappedMemory/blake3/blake3.h
        src/mappedMemory/blake3/blake3.c
		src/mappedMemory/blake3/blake3_dispatch.c
		src/mappedMemory/blake3/blake3_portable.c
		src/mappedMemory/blake3/blake3_impl.h
		src/mappedMemory/blake3/blake3_sse2_x86-64_unix.S
		src/mappedMemory/blake3/blake3_sse41_x86-64_unix.S
		src/mappedMemory/blake3/blake3_avx2_x86-64_unix.S
		src/mappedMemory/blake3/blake3_avx512_x86-64_unix.S
	)



set(UNSTDLIB_SRC
                src/unstdLib/errors.h
                src/unstdLib/math.h
                src/unstdLib/vars.h)

# set(R_SRC       src/ufos.h
#                 src/ufos.c
#                 src/init.c)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

# find_package(R REQUIRED)
# message("R libraries: ${R_LIBRARIES}")
# message("R include dirs: ${R_INCLUDE_DIR}")

find_package(Threads REQUIRED)
# find_package(OpenSSL REQUIRED)
# set(OPENSSL_TARGET OpenSSL::Crypto)
# message("R OpenSSL: ${OPENSSL_TARGET}")

#Rust Core
add_subdirectory(rust/ufos_c/)

add_library(blake3 SHARED ${BLAKE_SRC})

add_library(ufos SHARED ${CORE_SRC} ${R_SRC} ${UNSTDLIB_SRC})
include_directories(ufos ${R_INCLUDE_DIR})
target_link_libraries(ufos blake3 ${R_LIBRARIES})

add_executable(ufoTest src/mappedMemory/ufoTest.c)
# target_link_libraries(ufoTest ${CMAKE_THREAD_LIBS_INIT})
include_directories(ufoTest ufos_c)
target_link_libraries(ufoTest ufos_c)


#add_executable(oroborosTest src/mappedMemory/oroboros.c src/mappedMemory/oroboros.h src/mappedMemory/oroborosTest.c)
#target_link_libraries(oroborosTest ${CMAKE_THREAD_LIBS_INIT})

add_executable(strideTest  src/mappedMemory/stride.c)
target_link_libraries(strideTest ufos ${CMAKE_THREAD_LIBS_INIT})

add_executable(sparseListTest src/mappedMemory/sparseListTest.c)
target_link_libraries(sparseListTest ufos blake3 ${CMAKE_THREAD_LIBS_INIT})

add_executable(userfaultSpeed src/mappedMemory/userfaultSpeed.c)
target_link_libraries(userfaultSpeed ufos ${CMAKE_THREAD_LIBS_INIT})

add_executable(ufoSpeedTest src/mappedMemory/ufoSpeedTest.c)
include_directories(ufoSpeedTest ufos_c )
target_link_libraries(ufoSpeedTest ufos_c )

