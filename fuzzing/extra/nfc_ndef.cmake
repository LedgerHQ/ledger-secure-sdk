include_guard()

add_library(nfc_ndef
            ${CMAKE_CURRENT_SOURCE_DIR}/../lib_nfc/src/nfc_ndef.c
)
target_include_directories(nfc_ndef PUBLIC
                                ${CMAKE_CURRENT_SOURCE_DIR}/../lib_nfc/include/
                                ${CMAKE_CURRENT_SOURCE_DIR}/../include/
                                )

# Define the executable and its properties
add_executable(fuzz_nfc_ndef ${CMAKE_CURRENT_SOURCE_DIR}/fuzzer_nfc_ndef.c)
target_compile_definitions(nfc_ndef PUBLIC HAVE_NFC HAVE_NDEF_SUPPORT)
target_compile_options(fuzz_nfc_ndef PUBLIC ${COMPILATION_FLAGS})
target_link_options(fuzz_nfc_ndef PUBLIC ${COMPILATION_FLAGS})
target_link_libraries(fuzz_nfc_ndef PUBLIC nfc_ndef)
