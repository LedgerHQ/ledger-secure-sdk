include_guard()
include(${BOLOS_SDK}/fuzzing/v2/macros/macros.cmake)
include(${BOLOS_SDK}/fuzzing/v2/libs/lib_io.cmake)
include(${BOLOS_SDK}/fuzzing/v2/libs/lib_glyphs.cmake)
include(${BOLOS_SDK}/fuzzing/v2/libs/lib_qrcode.cmake)

file(GLOB LIB_NBGL_SOURCES "${BOLOS_SDK}/lib_nbgl/src/*.c"
     "${BOLOS_SDK}/lib_ux_nbgl/*.c")
# Exclude nbgl_use_case*.c — the fuzzing framework provides auto-approve
# stubs (fuzzing/v2/mock/nbgl/nbgl_use_case.c) so that review callbacks are
# invoked immediately and post-approval code paths are reachable.
list(FILTER LIB_NBGL_SOURCES EXCLUDE REGEX "nbgl_use_case[^/]*\\.c$")
add_library(ledger_fuzz_nbgl ${LIB_NBGL_SOURCES})
target_link_libraries(ledger_fuzz_nbgl PUBLIC ledger_fuzz_macros ledger_fuzz_glyphs ledger_fuzz_qrcode ledger_fuzz_mock ledger_fuzz_io)
target_compile_options(ledger_fuzz_nbgl PRIVATE ${COMPILATION_FLAGS})
target_include_directories(
  ledger_fuzz_nbgl
  PUBLIC "${BOLOS_SDK}/fuzzing/glyphs/"
         "${BOLOS_SDK}/lib_cxng/include/"
         "${BOLOS_SDK}/include/"
         "${BOLOS_SDK}/lib_ux_nbgl/"
         "${BOLOS_SDK}/lib_nbgl/include/"
         "${BOLOS_SDK}/lib_nbgl/src/"
         "${BOLOS_SDK}/target/${TARGET}/include")
