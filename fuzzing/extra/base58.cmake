include_guard()

# Include required liraries
include(${CMAKE_SOURCE_DIR}/libs/lib_standard_app.cmake)

# Define the executable and its properties
add_executable(fuzz_base58  ${CMAKE_SOURCE_DIR}/harness/fuzzer_base58.c)
target_compile_options(fuzz_base58  PUBLIC ${COMPILATION_FLAGS})
target_link_options(fuzz_base58  PUBLIC ${COMPILATION_FLAGS})

# Link with required libraries
target_link_libraries(fuzz_base58  PUBLIC lib_standard_app)
