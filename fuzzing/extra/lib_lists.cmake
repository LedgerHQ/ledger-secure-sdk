include_guard()

# Builds fuzz_lists from fuzzing/harness/fuzzer_lists.c.
include(${BOLOS_SDK}/fuzzing/libs/lib_lists.cmake)

add_executable(fuzz_lists ${BOLOS_SDK}/fuzzing/harness/fuzzer_lists.c)
target_compile_options(fuzz_lists PUBLIC ${COMPILATION_FLAGS})
target_link_options(fuzz_lists PUBLIC ${LINK_FLAGS})
target_link_libraries(fuzz_lists PUBLIC lists mock)
