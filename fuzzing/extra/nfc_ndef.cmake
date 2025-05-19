include_guard()

include(${BOLOS_SDK}/fuzzing/mock/mock.cmake)
include(${BOLOS_SDK}/fuzzing/libs/lib_nfc.cmake)

# Commented lines need os mock or depend of another function that needs a mock
file(GLOB LIB_NFC_SOURCES "${BOLOS_SDK}/lib_nfc/src/nfc_ndef.c")

# Define the executable and its properties
add_executable(fuzz_nfc_ndef ${BOLOS_SDK}/fuzzing/harness/fuzzer_nfc_ndef.c)
target_compile_options(fuzz_nfc_ndef PUBLIC ${COMPILATION_FLAGS})
target_link_options(fuzz_nfc_ndef PUBLIC ${COMPILATION_FLAGS})

# Link with required libraries
target_link_libraries(fuzz_nfc_ndef PUBLIC nfc)