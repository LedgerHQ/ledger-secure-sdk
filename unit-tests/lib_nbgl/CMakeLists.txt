cmake_minimum_required(VERSION 3.10)
project(unit_tests_lib_nbgl LANGUAGES C)

set(SDK_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../..)
include(${SDK_DIR}/cmake/LedgerUT.cmake)

ledger_unit_tests_init()

# Nano (non touch) build of the NBGL use cases: UT_COMPILE_DEFS_SDK targets Flex, so the
# Nano defines are given here explicitly.
ledger_unit_tests_add_test(
    NAME            test_nbgl_use_case_nanos
    SOURCES         ${SDK_DIR}/lib_nbgl/src/nbgl_use_case_nanos.c
    INCLUDE_DIRS    ${CMAKE_CURRENT_SOURCE_DIR}
                    ${SDK_DIR}
                    ${SDK_DIR}/include
                    ${SDK_DIR}/io/include
                    ${SDK_DIR}/lib_cxng/include
                    ${SDK_DIR}/lib_nbgl/include
                    ${SDK_DIR}/lib_nbgl/src
                    ${SDK_DIR}/lib_ux_nbgl
                    ${SDK_DIR}/target/nanox/include
    COMPILE_DEFS    TARGET_NANOX
                    OS_UNIT_TEST
                    HAVE_NBGL
                    NBGL_STEP
                    NBGL_USE_CASE
                    SCREEN_SIZE_NANO
                    BAGL_WIDTH=128
                    BAGL_HEIGHT=64
                    noreturn=
                    __IO=volatile
                    IO_HID_EP_LENGTH=64
                    OS_IO_SEPH_BUFFER_SIZE=272
                    APPNAME="UnitTests"
    COMPILE_OPTIONS -fsanitize=address,undefined
    LINK_OPTIONS    -fsanitize=address,undefined
)
