project(unit_tests_src LANGUAGES C)

# LedgerUT and ledger_unit_tests_init() live in the root CMakeLists.txt.

ledger_unit_tests_add_test(
    NAME            test_print
    SOURCES         ${LEDGER_SDK_ROOT}/src/os_printf.c
    MOCK_HEADERS    ${LEDGER_SDK_ROOT}/io/include/os_io_seph_cmd.h
    INCLUDE_DIRS    ${CMAKE_CURRENT_SOURCE_DIR}
                    /usr/include/bsd
                    ${UT_INCLUDE_DIRS_SDK}
    COMPILE_DEFS    ${UT_COMPILE_DEFS_SDK}
                    LIBBSD_OVERLAY
                    HAVE_PRINTF
                    HAVE_SPRINTF
)
target_link_libraries(test_print PRIVATE bsd)

ledger_unit_tests_add_test(
    NAME         test_hex
    SOURCES      ${LEDGER_SDK_ROOT}/src/os.c
                 ${CMAKE_CURRENT_SOURCE_DIR}/os_utils_stubs.c
                 ${LEDGER_UT_EXCEPTION_STUBS}
    INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
                 ${UT_INCLUDE_DIRS_SDK}
    COMPILE_DEFS ${UT_COMPILE_DEFS_SDK}
)

# os.c linked with the framework's exception stubs
ledger_unit_tests_add_test(
    NAME         test_os_longjmp
    SOURCES      ${LEDGER_SDK_ROOT}/src/os.c
                 ${CMAKE_CURRENT_SOURCE_DIR}/os_utils_stubs.c
                 ${LEDGER_UT_EXCEPTION_STUBS}
    INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
                 ${UT_INCLUDE_DIRS_SDK}
    COMPILE_DEFS ${UT_COMPILE_DEFS_SDK}
)

ledger_unit_tests_add_test(
    NAME         test_exceptions
    SOURCES      ${LEDGER_UT_EXCEPTION_STUBS}
    INCLUDE_DIRS ${UT_INCLUDE_DIRS_SDK}
    COMPILE_DEFS ${UT_COMPILE_DEFS_SDK}
)
