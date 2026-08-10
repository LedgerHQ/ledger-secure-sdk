cmake_minimum_required(VERSION 3.10)
project(unit_tests_app_storage LANGUAGES C)

set(SDK_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../..)
include(${SDK_DIR}/cmake/LedgerUT.cmake)

ledger_unit_tests_init()

ledger_unit_tests_add_test(
    NAME            test_app_storage
    SOURCES         ${SDK_DIR}/lib_standard_app/app_storage.c
    MOCK_HEADERS    ${SDK_DIR}/include/os_nvm.h
                    ${SDK_DIR}/lib_cxng/include/lcx_crc.h
    INCLUDE_DIRS    ${CMAKE_CURRENT_SOURCE_DIR}
                    ${UT_INCLUDE_DIRS_SDK}
    COMPILE_DEFS    ${UT_COMPILE_DEFS_SDK}
                    UNIT_TESTING
                    WITH_STDIO
                    HAVE_APP_STORAGE
                    APP_STORAGE_SIZE=5555
                    HAVE_APP_STORAGE_PROP_SETTINGS
                    HAVE_APP_STORAGE_PROP_DATA
    COMPILE_OPTIONS -fsanitize=address,undefined
    LINK_OPTIONS    -fsanitize=address,undefined
)
