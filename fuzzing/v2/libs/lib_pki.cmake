include_guard()
include(${BOLOS_SDK}/fuzzing/v2/macros/macros.cmake)
include(${BOLOS_SDK}/fuzzing/v2/libs/lib_cxng.cmake)

file(GLOB LIB_PKI_SOURCES "${BOLOS_SDK}/lib_pki/*.c")

add_library(ledger_fuzz_pki ${LIB_PKI_SOURCES})
target_compile_options(ledger_fuzz_pki PRIVATE ${COMPILATION_FLAGS})
target_link_libraries(ledger_fuzz_pki PUBLIC ledger_fuzz_macros ledger_fuzz_mock ledger_fuzz_cxng)
target_include_directories(
  ledger_fuzz_pki
  PUBLIC "${BOLOS_SDK}/include/" "${BOLOS_SDK}/target/${TARGET}/include"
         "${BOLOS_SDK}/lib_pki/" "${BOLOS_SDK}/lib_cxng/include")
