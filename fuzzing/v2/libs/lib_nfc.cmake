include_guard()
include(${BOLOS_SDK}/fuzzing/v2/macros/macros.cmake)
include(${BOLOS_SDK}/fuzzing/v2/libs/lib_io.cmake)

file(GLOB LIB_NFC_SOURCES "${BOLOS_SDK}/lib_nfc/src/*.c")

add_library(ledger_fuzz_nfc ${LIB_NFC_SOURCES})
target_link_libraries(ledger_fuzz_nfc PUBLIC ledger_fuzz_macros ledger_fuzz_io)
target_compile_options(ledger_fuzz_nfc PRIVATE ${COMPILATION_FLAGS})
target_include_directories(ledger_fuzz_nfc PUBLIC "${BOLOS_SDK}/include/"
                                                  "${BOLOS_SDK}/lib_nfc/include/")
