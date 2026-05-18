include_guard()
include(${BOLOS_SDK}/fuzzing/v2/macros/macros.cmake)

file(GLOB LIB_ALLOC_SOURCES "${BOLOS_SDK}/lib_alloc/*.c")

add_library(ledger_fuzz_alloc ${LIB_ALLOC_SOURCES})
target_link_libraries(ledger_fuzz_alloc PUBLIC ledger_fuzz_macros)
target_compile_options(ledger_fuzz_alloc PRIVATE ${COMPILATION_FLAGS})
target_include_directories(
  ledger_fuzz_alloc PUBLIC "${BOLOS_SDK}/include/" "${BOLOS_SDK}/lib_alloc/"
                           "${BOLOS_SDK}/target/${TARGET}/include")
