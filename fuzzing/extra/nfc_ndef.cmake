include_guard()

# Include required definitions
add_definitions(-DHAVE_NFC -DHAVE_NDEF_SUPPORT)

# Include required liraries
include(${CMAKE_SOURCE_DIR}/libs/lib_nfc.cmake)

# Define the executable and its properties
add_executable(fuzz_nfc_ndef ${CMAKE_SOURCE_DIR}/harness/fuzzer_nfc_ndef.c)
target_compile_options(fuzz_nfc_ndef PUBLIC ${COMPILATION_FLAGS})
target_link_options(fuzz_nfc_ndef PUBLIC ${COMPILATION_FLAGS})

# Link with required libraries
target_link_libraries(fuzz_nfc_ndef PUBLIC lib_nfc)
