cmake_minimum_required(VERSION 3.10)
project(unit_tests_io_legacy LANGUAGES C)

set(SDK_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../..)
include(${SDK_DIR}/cmake/LedgerUT.cmake)

ledger_unit_tests_init()

# The in-app IO stack, where a USB class can still read a reply from the app's buffer.
set(IO_LEGACY_COMPILE_DEFS ${UT_COMPILE_DEFS_SDK})
list(REMOVE_ITEM IO_LEGACY_COMPILE_DEFS USE_OS_IO_STACK)

# os_io_legacy.c is included by the test to reach its static state.
ledger_unit_tests_add_test(
    NAME            test_os_io_legacy
    INCLUDE_DIRS    ${CMAKE_CURRENT_SOURCE_DIR} ${UT_INCLUDE_DIRS_SDK} ${SDK_DIR}/io_legacy/src
    COMPILE_DEFS    ${IO_LEGACY_COMPILE_DEFS}
)
