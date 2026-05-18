include_guard()
include(${BOLOS_SDK}/fuzzing/v2/macros/macros.cmake)

file(GLOB LIB_LISTS_SOURCES "${BOLOS_SDK}/lib_lists/*.c")

add_library(ledger_fuzz_lists ${LIB_LISTS_SOURCES})
target_link_libraries(ledger_fuzz_lists PUBLIC ledger_fuzz_macros)
target_compile_options(ledger_fuzz_lists PRIVATE ${COMPILATION_FLAGS})
target_include_directories(
  ledger_fuzz_lists PUBLIC "${BOLOS_SDK}/include/" "${BOLOS_SDK}/lib_lists/"
                           "${BOLOS_SDK}/target/${TARGET}/include")
