include_guard()

# Builds fuzz_apdu_parser from fuzzing/harness/fuzzer_apdu_parser.c.
include(${BOLOS_SDK}/fuzzing/libs/lib_standard_app.cmake)

add_executable(fuzz_apdu_parser ${BOLOS_SDK}/fuzzing/harness/fuzzer_apdu_parser.c)
target_compile_options(fuzz_apdu_parser PUBLIC ${COMPILATION_FLAGS})
target_link_options(fuzz_apdu_parser PUBLIC ${LINK_FLAGS})
target_link_libraries(fuzz_apdu_parser PUBLIC standard_app mock)
