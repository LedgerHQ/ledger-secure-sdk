include_guard()

# Builds fuzz_bip32 from the harness under fuzzing/harness/, linked through
# the SDK fuzz library/mock environment.
include(${BOLOS_SDK}/fuzzing/libs/lib_standard_app.cmake)

add_executable(fuzz_bip32 ${BOLOS_SDK}/fuzzing/harness/fuzzer_bip32.c)
target_compile_options(fuzz_bip32 PUBLIC ${COMPILATION_FLAGS})
target_link_options(fuzz_bip32 PUBLIC ${LINK_FLAGS})
target_link_libraries(fuzz_bip32 PUBLIC standard_app mock)
