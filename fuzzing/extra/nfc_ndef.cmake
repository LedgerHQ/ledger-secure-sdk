include_guard()

add_definitions(-DHAVE_NFC -DHAVE_NDEF_SUPPORT)

add_library(nfc_ndef
            ${CMAKE_CURRENT_SOURCE_DIR}/../lib_nfc/src/nfc_ndef.c
)
target_include_directories(nfc_ndef PUBLIC
                                ${CMAKE_CURRENT_SOURCE_DIR}/../lib_nfc/include/
                                ${CMAKE_CURRENT_SOURCE_DIR}/../include/
                                )

set_target_properties(nfc_ndef PROPERTIES SOVERSION 1)

# Define the executable and its properties
add_executable(fuzz_nfc_ndef ${CMAKE_CURRENT_SOURCE_DIR}/fuzzer_nfc_ndef.c)
target_compile_options(fuzz_nfc_ndef PUBLIC ${COMPILATION_FLAGS})
target_link_options(fuzz_nfc_ndef PUBLIC ${COMPILATION_FLAGS})
target_link_libraries(fuzz_nfc_ndef PUBLIC nfc_ndef)
