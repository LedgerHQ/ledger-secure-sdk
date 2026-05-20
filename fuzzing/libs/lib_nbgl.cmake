include_guard()
include(${BOLOS_SDK}/fuzzing/macros/macros.cmake)
include(${BOLOS_SDK}/fuzzing/libs/lib_io.cmake)
include(${BOLOS_SDK}/fuzzing/libs/lib_glyphs.cmake)
include(${BOLOS_SDK}/fuzzing/libs/lib_qrcode.cmake)

file(GLOB LIB_NBGL_SOURCES "${BOLOS_SDK}/lib_nbgl/src/*.c"
     "${BOLOS_SDK}/lib_ux_nbgl/*.c")
# Exclude nbgl_use_case*.c — the fuzzing framework provides auto-approve
# stubs (fuzzing/mock/nbgl/nbgl_use_case.c) so that review callbacks are
# invoked immediately and post-approval code paths are reachable.
list(FILTER LIB_NBGL_SOURCES EXCLUDE REGEX "nbgl_use_case[^/]*\\.c$")
add_library(nbgl ${LIB_NBGL_SOURCES})
target_link_libraries(nbgl PUBLIC macros glyphs qrcode mock io)
target_compile_options(nbgl PRIVATE ${COMPILATION_FLAGS})
target_include_directories(
  nbgl
  PUBLIC "${BOLOS_SDK}/fuzzing/glyphs/"
         "${BOLOS_SDK}/lib_cxng/include/"
         "${BOLOS_SDK}/include/"
         "${BOLOS_SDK}/lib_ux_nbgl/"
         "${BOLOS_SDK}/lib_nbgl/include/"
         "${BOLOS_SDK}/lib_nbgl/src/"
         "${BOLOS_SDK}/target/${TARGET}/include")
