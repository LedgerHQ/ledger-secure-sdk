project(unit_tests_io_legacy LANGUAGES C)

# LedgerUT and ledger_unit_tests_init() live in the root CMakeLists.txt.

# os_io_legacy.c is included by the test to reach its static state.
ledger_unit_tests_add_test(
    NAME            test_os_io_legacy
    INCLUDE_DIRS    ${CMAKE_CURRENT_SOURCE_DIR} ${UT_INCLUDE_DIRS_SDK} ${LEDGER_SDK_ROOT}/io_legacy/src
    COMPILE_DEFS    ${UT_COMPILE_DEFS_SDK}
    # The in-app IO stack, where a USB class can still read a reply from the app's buffer.
    COMPILE_OPTIONS -UUSE_OS_IO_STACK
)
