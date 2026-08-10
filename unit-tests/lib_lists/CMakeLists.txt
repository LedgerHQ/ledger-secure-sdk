cmake_minimum_required(VERSION 3.10)
project(unit_tests_lib_lists LANGUAGES C)

set(SDK_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../..)
include(${SDK_DIR}/cmake/LedgerUT.cmake)

ledger_unit_tests_init()

ledger_unit_tests_add_test(
    NAME         test_lists
    SOURCES      ${SDK_DIR}/lib_lists/lists.c
    INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
                 ${SDK_DIR}/lib_lists
                 ${UT_INCLUDE_DIRS_SDK}
    COMPILE_DEFS ${UT_COMPILE_DEFS_SDK}
)
