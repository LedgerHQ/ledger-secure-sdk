include_guard()

# Include required libraries
include(${BOLOS_SDK}/fuzzing/libs/lib_c_list.cmake)
include(${BOLOS_SDK}/fuzzing/mock/mock.cmake)

# Define the executable and its properties here
add_executable(fuzz_c_list ${BOLOS_SDK}/fuzzing/harness/fuzzer_c_list.c)
target_compile_options(fuzz_c_list PUBLIC ${COMPILATION_FLAGS})
target_link_options(fuzz_c_list PUBLIC ${LINK_FLAGS})

# Link with required libraries
target_link_libraries(fuzz_c_list PUBLIC c_list mock)
