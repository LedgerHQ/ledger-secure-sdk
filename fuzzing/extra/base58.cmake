include_guard()

# Include required liraries
include(${BOLOS_SDK}/fuzzing/libs/lib_standard_app.cmake)

# Define the executable and its properties
add_executable(fuzz_base58 ${BOLOS_SDK}/fuzzing/harness/fuzzer_base58.c ${BOLOS_SDK}/lib_alloc/mem_alloc.c)
target_compile_options(fuzz_base58 PUBLIC ${COMPILATION_FLAGS})
target_link_options(fuzz_base58 PUBLIC ${LINK_FLAGS})

# Link with required libraries
target_link_libraries(fuzz_base58 PUBLIC standard_app nbgl_shared)
