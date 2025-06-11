# -----------------------------------------------------------------------------------
# IMPORTANT NOTE:
#
# When globbing source files from the mock directory, it is **critical** that
# the "custom" folder appears **alphabetically before** the "generated" folder.
#
# This is because the symbols are **statically linked**, and the linker resolves
# symbols in the **order they appear** in the source list. By ensuring that the
# custom implementations come first, we allow them to **override** the symbols
# from the generated mocks.
#
# Failing to maintain this order may result in the linker choosing the wrong
# symbol, and using the generated mocks instead of the custom ones.
# -----------------------------------------------------------------------------------
include_guard()

include(${BOLOS_SDK}/fuzzing/macros/macros.cmake)
include(${BOLOS_SDK}/fuzzing/libs/lib_nbgl.cmake)
include(${BOLOS_SDK}/fuzzing/libs/lib_standard_app.cmake)
include(${BOLOS_SDK}/fuzzing/libs/lib_cxng.cmake)

file(
  GLOB
  LIB_MOCK_SOURCES
  "${BOLOS_SDK}/src/os.c"
  "${BOLOS_SDK}/src/ledger_assert.c"
  "${BOLOS_SDK}/src/cx_wrappers.c"
  "${BOLOS_SDK}/src/cx_hash_iovec.c"
  "${BOLOS_SDK}/fuzzing/mock/custom/*.c"
  "${BOLOS_SDK}/fuzzing/mock/generated/*.c")
add_library(mock ${LIB_MOCK_SOURCES})

target_link_libraries(mock PUBLIC macros cxng nbgl standard_app)
target_compile_options(mock PUBLIC ${COMPILATION_FLAGS})
target_include_directories(
  mock
  PUBLIC ${BOLOS_SDK}/lib_cxng/include/
         ${BOLOS_SDK}/target/${TARGET_DEVICE}/include/
         ${BOLOS_SDK}/lib_standard_app/
         ${BOLOS_SDK}/include/
         ${BOLOS_SDK}/fuzzing/mock/written/
         ${BOLOS_SDK}/target/${TARGET_DEVICE}/)
