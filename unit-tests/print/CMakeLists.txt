cmake_minimum_required(VERSION 3.10)
project(unit_tests_print LANGUAGES C)

set(SDK_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../..)
include(${SDK_DIR}/cmake/LedgerUT.cmake)

ledger_unit_tests_init()

ledger_unit_tests_add_test(
    NAME            test_print
    SOURCES         ${SDK_DIR}/src/os_printf.c
    MOCK_HEADERS    ${SDK_DIR}/io/include/os_io_seph_cmd.h
    INCLUDE_DIRS    ${CMAKE_CURRENT_SOURCE_DIR}
                    /usr/include/bsd
                    ${UT_INCLUDE_DIRS_SDK}
    COMPILE_DEFS    ${UT_COMPILE_DEFS_SDK}
                    LIBBSD_OVERLAY
                    HAVE_PRINTF
                    HAVE_SPRINTF
    COMPILE_OPTIONS -fsanitize=address,undefined
    LINK_OPTIONS    -fsanitize=address,undefined
)
target_link_libraries(test_print PRIVATE bsd)
