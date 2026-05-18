include_guard()
include(${BOLOS_SDK}/fuzzing/v2/macros/macros.cmake)
include(${BOLOS_SDK}/fuzzing/v2/libs/lib_cxng.cmake)
include(${BOLOS_SDK}/fuzzing/v2/libs/lib_io.cmake)
include(${BOLOS_SDK}/fuzzing/v2/libs/lib_nfc.cmake)

file(GLOB LIB_STANDARD_APP_SOURCES "${BOLOS_SDK}/lib_standard_app/*.c" "${BOLOS_SDK}/src/os_printf.c")
list(REMOVE_ITEM LIB_STANDARD_APP_SOURCES
     "${BOLOS_SDK}/lib_standard_app/main.c")
add_library(ledger_fuzz_standard_app ${LIB_STANDARD_APP_SOURCES})
target_link_libraries(ledger_fuzz_standard_app PUBLIC ledger_fuzz_macros ledger_fuzz_mock ledger_fuzz_cxng ledger_fuzz_io ledger_fuzz_nfc)
target_compile_options(ledger_fuzz_standard_app PRIVATE ${COMPILATION_FLAGS})
target_include_directories(
  ledger_fuzz_standard_app
  PUBLIC "${BOLOS_SDK}/include/" "${BOLOS_SDK}/target/${TARGET}/include"
         "${BOLOS_SDK}/lib_cxng" "${BOLOS_SDK}/lib_cxng/include"
         "${BOLOS_SDK}/lib_standard_app")
