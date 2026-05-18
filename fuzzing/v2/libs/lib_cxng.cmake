include_guard()
include(${BOLOS_SDK}/fuzzing/v2/macros/macros.cmake)
include(${BOLOS_SDK}/fuzzing/v2/mock/mock.cmake)

file(GLOB LIB_CXNG_SOURCES "${BOLOS_SDK}/lib_cxng/src/*.c")

add_library(ledger_fuzz_cxng ${LIB_CXNG_SOURCES} "${BOLOS_SDK}/src/cx_hash_iovec.c")
target_compile_options(ledger_fuzz_cxng PRIVATE ${COMPILATION_FLAGS})
target_compile_definitions(ledger_fuzz_cxng PUBLIC HAVE_FIXED_SCALAR_LENGTH)
target_link_libraries(ledger_fuzz_cxng PUBLIC ledger_fuzz_macros ledger_fuzz_mock)
target_include_directories(
  ledger_fuzz_cxng
  PUBLIC "${BOLOS_SDK}/include/" "${BOLOS_SDK}/target/${TARGET}/include"
         "${BOLOS_SDK}/lib_cxng/" "${BOLOS_SDK}/lib_cxng/include"
         "${BOLOS_SDK}/lib_cxng/src" "${BOLOS_SDK}/")
