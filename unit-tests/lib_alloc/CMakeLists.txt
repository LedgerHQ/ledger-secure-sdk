cmake_minimum_required(VERSION 3.10)
project(unit_tests_lib_alloc LANGUAGES C)

set(SDK_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../..)
include(${SDK_DIR}/cmake/LedgerUT.cmake)

ledger_unit_tests_init()

ledger_unit_tests_add_test(
    NAME         test_mem_alloc
    SOURCES      ${SDK_DIR}/lib_alloc/mem_alloc.c
    INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
                 ${SDK_DIR}/lib_alloc
                 ${UT_INCLUDE_DIRS_SDK}
    COMPILE_DEFS ${UT_COMPILE_DEFS_SDK}
    LINK_OPTIONS -Wl,--wrap=os_longjmp
)

ledger_unit_tests_add_test(
    NAME         test_app_mem_utils
    SOURCES      ${SDK_DIR}/lib_alloc/app_mem_utils.c
    MOCK_HEADERS ${SDK_DIR}/lib_alloc/mem_alloc.h
    INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
                 ${SDK_DIR}/lib_alloc
                 ${UT_INCLUDE_DIRS_SDK}
    COMPILE_DEFS ${UT_COMPILE_DEFS_SDK}
)
