project(ufo)
cmake_minimum_required(VERSION 2.8.7)


SET( EX_PLATFORM 64 )
SET( EX_PLATFORM_NAME "x64" )

add_definitions(
  -std=c++11 -Wall -O2
#  -std=c++11 -Wall -g
)

#find_package(Threads)
##find_package(Boost REQUIRED COMPONENTS system)
#
##include_directories(${Boost_INCLUDE_DIR})
#include_directories(".")
#
file(GLOB_RECURSE SRC "src/*.c" "src/*.h")
#
#add_executable(dejavu ${SRC})
#target_link_libraries(${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT})
##target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES} rt)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

find_package(R REQUIRED)
message("R libraries: ${R_LIBRARIES}")
message("R include dirs: ${R_INCLUDE_DIR} ${Rcpp_INCLUDE_DIR}")

add_library(ufos SHARED ${SRC})
include_directories(ufos ${R_INCLUDE_DIR} ${Rcpp_INCLUDE_DIR})
target_link_libraries(ufos ${R_LIBRARIES})

#install(TARGETS ufos DESTINATION lib)
