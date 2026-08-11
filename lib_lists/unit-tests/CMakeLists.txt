project(unit_tests_lib_lists LANGUAGES C)

# LedgerUT and ledger_unit_tests_init() live in the root CMakeLists.txt.

ledger_unit_tests_add_test(
    NAME         test_lists
    SOURCES      ${LEDGER_SDK_ROOT}/lib_lists/lists.c
    INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
                 ${LEDGER_SDK_ROOT}/lib_lists
                 ${UT_INCLUDE_DIRS_SDK}
    COMPILE_DEFS ${UT_COMPILE_DEFS_SDK}
)
