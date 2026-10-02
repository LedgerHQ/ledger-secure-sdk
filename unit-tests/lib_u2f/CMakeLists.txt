cmake_minimum_required(VERSION 3.10)
project(unit_tests_lib_u2f LANGUAGES C)

set(SDK_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../..)
include(${SDK_DIR}/cmake/LedgerUT.cmake)

ledger_unit_tests_init()

set(U2F_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${UT_INCLUDE_DIRS_SDK}
)

set(U2F_COMPILE_DEFS
    ${UT_COMPILE_DEFS_SDK}
)

set(U2F_COMPILE_OPTIONS -fsanitize=address,undefined)
set(U2F_LINK_OPTIONS    -fsanitize=address,undefined)

ledger_unit_tests_add_test(
    NAME            test_u2f_transport
    SOURCES         ${SDK_DIR}/lib_u2f/src/u2f_transport.c
    INCLUDE_DIRS    ${U2F_INCLUDE_DIRS}
    COMPILE_DEFS    ${U2F_COMPILE_DEFS}
    COMPILE_OPTIONS ${U2F_COMPILE_OPTIONS}
    LINK_OPTIONS    ${U2F_LINK_OPTIONS}
)

# usbd_ledger_hid_u2f.c is included by the test to reach its static handle.
ledger_unit_tests_add_test(
    NAME            test_usbd_ledger_hid_u2f
    SOURCES         ${SDK_DIR}/lib_u2f/src/u2f_transport.c
    MOCK_HEADERS    ${SDK_DIR}/lib_stusb/include/usbd_core.h
                    ${SDK_DIR}/lib_stusb/include/usbd_ioreq.h
                    ${SDK_DIR}/lib_cxng/include/lcx_crc.h
    INCLUDE_DIRS    ${U2F_INCLUDE_DIRS} ${SDK_DIR}/lib_stusb/src
    COMPILE_DEFS    ${U2F_COMPILE_DEFS}
    COMPILE_OPTIONS ${U2F_COMPILE_OPTIONS}
    LINK_OPTIONS    ${U2F_LINK_OPTIONS}
)
