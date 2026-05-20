include_guard()

# Builds fuzz_base58 from fuzzing/harness/fuzzer_base58.c.
include(${BOLOS_SDK}/fuzzing/libs/lib_standard_app.cmake)

add_executable(fuzz_base58 ${BOLOS_SDK}/fuzzing/harness/fuzzer_base58.c)
target_compile_options(fuzz_base58 PUBLIC ${COMPILATION_FLAGS})
target_link_options(fuzz_base58 PUBLIC ${LINK_FLAGS})
target_link_libraries(fuzz_base58 PUBLIC standard_app mock)
