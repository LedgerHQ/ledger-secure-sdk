include_guard()

# Include required liraries
include(${CMAKE_SOURCE_DIR}/libs/lib_qrcode.cmake)
include(${CMAKE_SOURCE_DIR}/mock/mock.cmake)

# Define the executable and its properties
add_executable(fuzz_qrcodegen ${CMAKE_SOURCE_DIR}/harness/fuzzer_qrcodegen.c)
target_compile_options(fuzz_qrcodegen PUBLIC ${COMPILATION_FLAGS})
target_link_options(fuzz_qrcodegen PUBLIC ${COMPILATION_FLAGS})

# Link with required libraries
target_link_libraries(fuzz_qrcodegen PUBLIC lib_qrcode lib_mock)