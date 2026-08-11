project(unit_tests_lib_u2f LANGUAGES C)

# LedgerUT and ledger_unit_tests_init() live in the root CMakeLists.txt.

set(U2F_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${UT_INCLUDE_DIRS_SDK}
)

set(U2F_COMPILE_DEFS
    ${UT_COMPILE_DEFS_SDK}
)

ledger_unit_tests_add_test(
    NAME            test_u2f_transport
    SOURCES         ${LEDGER_SDK_ROOT}/lib_u2f/src/u2f_transport.c
    INCLUDE_DIRS    ${U2F_INCLUDE_DIRS}
    COMPILE_DEFS    ${U2F_COMPILE_DEFS}
)

# usbd_ledger_hid_u2f.c is included by the test to reach its static handle.
ledger_unit_tests_add_test(
    NAME            test_usbd_ledger_hid_u2f
    SOURCES         ${LEDGER_SDK_ROOT}/lib_u2f/src/u2f_transport.c
    MOCK_HEADERS    ${LEDGER_SDK_ROOT}/lib_stusb/include/usbd_core.h
                    ${LEDGER_SDK_ROOT}/lib_stusb/include/usbd_ioreq.h
                    ${LEDGER_SDK_ROOT}/lib_cxng/include/lcx_crc.h
    INCLUDE_DIRS    ${U2F_INCLUDE_DIRS} ${LEDGER_SDK_ROOT}/lib_stusb/src
    COMPILE_DEFS    ${U2F_COMPILE_DEFS}
)
