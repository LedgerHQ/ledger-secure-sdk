include_guard()

# Include required liraries
include(${BOLOS_SDK}/fuzzing/libs/lib_standard_app.cmake)

# Define the executable and its properties
add_executable(fuzz_bip32 ${BOLOS_SDK}/fuzzing/harness/fuzzer_bip32.c)
target_compile_options(fuzz_bip32 PUBLIC ${COMPILATION_FLAGS})
target_link_options(fuzz_bip32 PUBLIC ${LINK_FLAGS})

# Link with required libraries
target_link_libraries(fuzz_bip32 PUBLIC standard_app nbgl_shared)
