include_guard()

# Builds fuzz_qrcodegen from fuzzing/harness/fuzzer_qrcodegen.c.
include(${BOLOS_SDK}/fuzzing/libs/lib_qrcode.cmake)

add_executable(fuzz_qrcodegen ${BOLOS_SDK}/fuzzing/harness/fuzzer_qrcodegen.c)
target_compile_options(fuzz_qrcodegen PUBLIC ${COMPILATION_FLAGS})
target_link_options(fuzz_qrcodegen PUBLIC ${LINK_FLAGS})
target_link_libraries(fuzz_qrcodegen PUBLIC qrcode mock)
