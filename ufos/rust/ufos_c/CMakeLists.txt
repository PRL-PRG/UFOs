if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CARGO_CMD cargo build)
    set(BIN_DIR "target/debug")
else ()
    set(CARGO_CMD cargo build --release)
    set(BIN_DIR "target/release")
endif ()

set(UFOS_C_LINK "${CMAKE_CURRENT_BINARY_DIR}/${BIN_DIR}/libufos_c.so")
set(UFOS_C_INCLUDE  "target/")

add_custom_target(ufos_c_cargo
    COMMENT "Compiling ufo rust core and c bindings"
    COMMAND ${CARGO_CMD} 
)

add_library(ufos_c INTERFACE)
target_link_libraries(ufos_c INTERFACE ${UFOS_C_LINK})
target_include_directories(ufos_c INTERFACE ${UFOS_C_INCLUDE})
add_dependencies(ufos_c ufos_c_cargo)

#add_test(NAME client_test 
#    COMMAND cargo test
#    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
