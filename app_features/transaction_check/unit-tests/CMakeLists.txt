project(unit_tests_transaction_check LANGUAGES C)

set(TC_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${LEDGER_SDK_ROOT}/app_features/transaction_check/include
    ${LEDGER_SDK_ROOT}/app_features/transaction_check/src
    ${LEDGER_SDK_ROOT}/app_features/common/include
    ${LEDGER_SDK_ROOT}/lib_tlv/use_cases
    ${UT_INCLUDE_DIRS_SDK}
)

set(TC_COMPILE_DEFS
    ${UT_COMPILE_DEFS_SDK}
    HAVE_TX_CHECK
    HAVE_TX_CHECK_APDU
    HAVE_SWAP
    APPNAME="UnitTests"
    WITH_STDIO
)

# transaction_check.c uses strlcpy (libbsd)
ledger_unit_tests_add_test(
    NAME         test_transaction_check
    SOURCES      ${LEDGER_SDK_ROOT}/app_features/transaction_check/src/transaction_check.c
                 ${LEDGER_SDK_ROOT}/app_features/common/src/apdu_chunks.c
    MOCK_HEADERS ${LEDGER_SDK_ROOT}/lib_standard_app/io.h
                 ${LEDGER_SDK_ROOT}/lib_nbgl/include/nbgl_use_case.h
    INCLUDE_DIRS ${TC_INCLUDE_DIRS}
                 /usr/include/bsd
    COMPILE_DEFS ${TC_COMPILE_DEFS}
                 LIBBSD_OVERLAY
    COMPILE_OPTIONS -UHAVE_ADDRESS_BOOK
)
target_link_libraries(test_transaction_check PRIVATE bsd)

ledger_unit_tests_add_test(
    NAME         test_transaction_check_ui
    SOURCES      ${LEDGER_SDK_ROOT}/app_features/transaction_check/src/transaction_check_ui.c
    MOCK_HEADERS ${LEDGER_SDK_ROOT}/lib_standard_app/io.h
                 ${LEDGER_SDK_ROOT}/lib_nbgl/include/nbgl_use_case.h
                 ${LEDGER_SDK_ROOT}/lib_nbgl/include/nbgl_layout.h
                 ${LEDGER_SDK_ROOT}/lib_nbgl/include/nbgl_obj.h
    INCLUDE_DIRS ${TC_INCLUDE_DIRS}
    COMPILE_DEFS ${TC_COMPILE_DEFS}
    COMPILE_OPTIONS -UHAVE_ADDRESS_BOOK
)

ledger_unit_tests_add_test(
    NAME         test_transaction_check_ui_default_reply
    SOURCES      ${LEDGER_SDK_ROOT}/app_features/transaction_check/src/transaction_check_ui.c
    MOCK_HEADERS ${LEDGER_SDK_ROOT}/lib_standard_app/io.h
                 ${LEDGER_SDK_ROOT}/lib_nbgl/include/nbgl_use_case.h
                 ${LEDGER_SDK_ROOT}/lib_nbgl/include/nbgl_layout.h
                 ${LEDGER_SDK_ROOT}/lib_nbgl/include/nbgl_obj.h
    INCLUDE_DIRS ${TC_INCLUDE_DIRS}
    COMPILE_DEFS ${TC_COMPILE_DEFS}
    COMPILE_OPTIONS -UHAVE_ADDRESS_BOOK
)
