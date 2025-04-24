include_guard()

add_library(bip32
    ${CMAKE_CURRENT_SOURCE_DIR}/../lib_standard_app/read.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../lib_standard_app/bip32.c
)
target_include_directories(bip32 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../lib_standard_app)

set_target_properties(bip32 PROPERTIES SOVERSION 1)

# Define the executable and its properties
add_executable(fuzz_bip32 ${CMAKE_CURRENT_SOURCE_DIR}/fuzzer_bip32.c)
target_compile_options(fuzz_bip32 PUBLIC ${COMPILATION_FLAGS})
target_link_options(fuzz_bip32 PUBLIC ${COMPILATION_FLAGS})
target_link_libraries(fuzz_bip32 PUBLIC bip32)

