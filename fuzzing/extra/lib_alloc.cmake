include_guard()

# Include required liraries
include(${BOLOS_SDK}/fuzzing/libs/lib_alloc.cmake)
include(${BOLOS_SDK}/fuzzing/libs/lib_nbgl.cmake)
include(${BOLOS_SDK}/fuzzing/libs/lib_io.cmake)
include(${BOLOS_SDK}/fuzzing/mock/mock.cmake)

# Define the executable and its properties here
add_executable(fuzz_alloc ${BOLOS_SDK}/fuzzing/harness/fuzzer_alloc.c)
target_compile_options(fuzz_alloc PUBLIC ${COMPILATION_FLAGS})
target_link_options(fuzz_alloc PUBLIC ${LINK_FLAGS})

# Link with required libraries
target_link_libraries(fuzz_alloc PUBLIC alloc nbgl io mock)

# Add fuzzer for alloc_utils high-level functions
add_executable(fuzz_alloc_utils ${BOLOS_SDK}/fuzzing/harness/fuzzer_alloc_utils.c)
target_compile_options(fuzz_alloc_utils PUBLIC ${COMPILATION_FLAGS})
target_link_options(fuzz_alloc_utils PUBLIC ${LINK_FLAGS})

# Link with required libraries
target_link_libraries(fuzz_alloc_utils PUBLIC alloc nbgl io mock)
