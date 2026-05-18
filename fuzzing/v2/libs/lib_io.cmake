include_guard()
include(${BOLOS_SDK}/fuzzing/v2/macros/macros.cmake)
include(${BOLOS_SDK}/fuzzing/v2/libs/lib_nbgl.cmake)
include(${BOLOS_SDK}/fuzzing/v2/libs/lib_cxng.cmake)
include(${BOLOS_SDK}/fuzzing/v2/libs/lib_nfc.cmake)

file(
  GLOB
  LIB_IO_SOURCES
  "${BOLOS_SDK}/io/src/*.c"
  "${BOLOS_SDK}/io_legacy/src/*.c"
  "${BOLOS_SDK}/lib_blewbxx/src/*.c"
  "${BOLOS_SDK}/lib_blewbxx_impl/src/*.c"
  "${BOLOS_SDK}/lib_ccid/src/*.c"
  "${BOLOS_SDK}/lib_stusb/src/*.c"
  "${BOLOS_SDK}/lib_stusb_impl/src/*.c"
  "${BOLOS_SDK}/lib_u2f/src/*.c"
  "${BOLOS_SDK}/lib_u2f_legacy/src/*.c"
  "${BOLOS_SDK}/protocol/src/*.c")

add_library(ledger_fuzz_io ${LIB_IO_SOURCES})
target_link_libraries(ledger_fuzz_io PUBLIC ledger_fuzz_nbgl ledger_fuzz_cxng ledger_fuzz_macros ledger_fuzz_nfc)
target_compile_options(ledger_fuzz_io PRIVATE ${COMPILATION_FLAGS})
target_compile_options(ledger_fuzz_io PUBLIC -Wno-implicit-function-declaration)
target_include_directories(
  ledger_fuzz_io
  PUBLIC "${BOLOS_SDK}/include/"
         "${BOLOS_SDK}/target/${TARGET}/"
         "${BOLOS_SDK}/target/${TARGET}/include/"
         "${BOLOS_SDK}/io/include"
         "${BOLOS_SDK}/io_legacy/include"
         "${BOLOS_SDK}/lib_blewbxx/include"
         "${BOLOS_SDK}/lib_blewbxx_impl/include"
         "${BOLOS_SDK}/lib_ccid/include"
         "${BOLOS_SDK}/lib_stusb/include"
         "${BOLOS_SDK}/lib_stusb_impl/include"
         "${BOLOS_SDK}/lib_u2f/include"
         "${BOLOS_SDK}/lib_u2f_legacy/include"
         "${BOLOS_SDK}/protocol/include")
