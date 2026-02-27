include_guard()

# Include required liraries
include(${BOLOS_SDK}/fuzzing/mock/mock.cmake)
include(${BOLOS_SDK}/fuzzing/libs/lib_qrcode.cmake)

# Define the executable and its properties
add_executable(fuzz_qrcodegen ${BOLOS_SDK}/fuzzing/harness/fuzzer_qrcodegen.c ${BOLOS_SDK}/lib_alloc/mem_alloc.c)
target_compile_options(fuzz_qrcodegen PUBLIC ${COMPILATION_FLAGS})
target_link_options(fuzz_qrcodegen PUBLIC ${LINK_FLAGS})

# Link with required libraries
target_link_libraries(fuzz_qrcodegen PUBLIC qrcode mock nbgl_shared)
