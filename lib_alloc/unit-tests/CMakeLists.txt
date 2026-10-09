project(unit_tests_lib_alloc LANGUAGES C)

# LedgerUT and ledger_unit_tests_init() live in the root CMakeLists.txt.

ledger_unit_tests_add_test(
    NAME         test_mem_alloc
    SOURCES      ${LEDGER_SDK_ROOT}/lib_alloc/mem_alloc.c
    INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
                 ${LEDGER_SDK_ROOT}/lib_alloc
                 ${UT_INCLUDE_DIRS_SDK}
    COMPILE_DEFS ${UT_COMPILE_DEFS_SDK}
    LINK_OPTIONS -Wl,--wrap=os_longjmp
)

ledger_unit_tests_add_test(
    NAME         test_app_mem_utils
    SOURCES      ${LEDGER_SDK_ROOT}/lib_alloc/app_mem_utils.c
    MOCK_HEADERS ${LEDGER_SDK_ROOT}/lib_alloc/mem_alloc.h
    INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
                 ${LEDGER_SDK_ROOT}/lib_alloc
                 ${UT_INCLUDE_DIRS_SDK}
    COMPILE_DEFS ${UT_COMPILE_DEFS_SDK}
)
