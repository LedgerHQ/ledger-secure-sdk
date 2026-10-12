cmake_minimum_required(VERSION 3.10)
project(unit_tests_io_legacy LANGUAGES C)

set(SDK_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../..)
include(${SDK_DIR}/cmake/LedgerUT.cmake)

ledger_unit_tests_init()

# os_io_legacy.c is included by the test to reach its static state.
ledger_unit_tests_add_test(
    NAME            test_os_io_legacy
    INCLUDE_DIRS    ${CMAKE_CURRENT_SOURCE_DIR} ${UT_INCLUDE_DIRS_SDK} ${SDK_DIR}/io_legacy/src
    COMPILE_DEFS    ${UT_COMPILE_DEFS_SDK}
    COMPILE_OPTIONS -fsanitize=address,undefined
    LINK_OPTIONS    -fsanitize=address,undefined
)
