include_guard()

add_executable(fuzz_nfc_ndef
               ${BOLOS_SDK}/fuzzing/harness/fuzzer_nfc_ndef.c
               ${BOLOS_SDK}/lib_nfc/src/nfc_ndef.c)
target_compile_options(fuzz_nfc_ndef PUBLIC ${COMPILATION_FLAGS})
target_link_options(fuzz_nfc_ndef PUBLIC ${LINK_FLAGS})
target_compile_definitions(fuzz_nfc_ndef PUBLIC HAVE_NFC HAVE_NDEF_SUPPORT)
target_include_directories(fuzz_nfc_ndef PUBLIC "${BOLOS_SDK}/include/"
                                                "${BOLOS_SDK}/lib_nfc/include/")
target_link_libraries(fuzz_nfc_ndef PUBLIC nbgl_shared)
