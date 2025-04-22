include_guard()

# Include required liraries
include(${CMAKE_SOURCE_DIR}/libs/lib_alloc.cmake)
include(${CMAKE_SOURCE_DIR}/mock/mock.cmake)

# Define the executable and its properties here
add_executable(fuzz_alloc ${CMAKE_SOURCE_DIR}/harness/fuzzer_alloc.c)
target_compile_options(fuzz_alloc PUBLIC ${COMPILATION_FLAGS})
target_link_options(fuzz_alloc PUBLIC ${COMPILATION_FLAGS})

# Link with required libraries
target_link_libraries(fuzz_alloc PUBLIC lib_alloc lib_mock)

