include_guard()
include(${BOLOS_SDK}/fuzzing/v2/macros/macros.cmake)

file(GLOB LIB_QRCODE_SOURCES "${BOLOS_SDK}/qrcode/src/*.c")

add_library(ledger_fuzz_qrcode ${LIB_QRCODE_SOURCES})
target_link_libraries(ledger_fuzz_qrcode PUBLIC ledger_fuzz_macros)
target_compile_options(ledger_fuzz_qrcode PRIVATE ${COMPILATION_FLAGS})
target_include_directories(
  ledger_fuzz_qrcode PUBLIC "${BOLOS_SDK}/include/" "${BOLOS_SDK}/qrcode/include/"
                            "${BOLOS_SDK}/target/${TARGET}/include/")
