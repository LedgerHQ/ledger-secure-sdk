cmake_minimum_required(VERSION 3.10)
project(unit_tests_os_utils LANGUAGES C)

set(SDK_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../..)
include(${SDK_DIR}/cmake/LedgerUT.cmake)

ledger_unit_tests_init()

ledger_unit_tests_add_test(
    NAME         test_hex
    SOURCES      ${SDK_DIR}/src/os.c
                 ${CMAKE_CURRENT_SOURCE_DIR}/stubs.c
    INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
                 ${UT_INCLUDE_DIRS_SDK}
    COMPILE_DEFS ${UT_COMPILE_DEFS_SDK}
)
