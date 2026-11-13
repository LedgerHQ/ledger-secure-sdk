cmake_minimum_required(VERSION 3.10)
project(unit_tests_transaction_check LANGUAGES C)

set(SDK_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../..)
include(${SDK_DIR}/cmake/LedgerUT.cmake)

ledger_unit_tests_init()

set(TC_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${SDK_DIR}/app_features/transaction_check/include
    ${SDK_DIR}/app_features/transaction_check/src
    ${SDK_DIR}/app_features/common/include
    ${SDK_DIR}/lib_tlv/use_cases
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
    SOURCES      ${SDK_DIR}/app_features/transaction_check/src/transaction_check.c
                 ${SDK_DIR}/app_features/common/src/apdu_chunks.c
    MOCK_HEADERS ${SDK_DIR}/lib_standard_app/io.h
                 ${SDK_DIR}/lib_nbgl/include/nbgl_use_case.h
    INCLUDE_DIRS ${TC_INCLUDE_DIRS}
                 /usr/include/bsd
    COMPILE_DEFS ${TC_COMPILE_DEFS}
                 LIBBSD_OVERLAY
)
target_link_libraries(test_transaction_check PRIVATE bsd)

ledger_unit_tests_add_test(
    NAME         test_transaction_check_ui
    SOURCES      ${SDK_DIR}/app_features/transaction_check/src/transaction_check_ui.c
    MOCK_HEADERS ${SDK_DIR}/lib_standard_app/io.h
                 ${SDK_DIR}/lib_nbgl/include/nbgl_use_case.h
                 ${SDK_DIR}/lib_nbgl/include/nbgl_layout.h
                 ${SDK_DIR}/lib_nbgl/include/nbgl_obj.h
    INCLUDE_DIRS ${TC_INCLUDE_DIRS}
    COMPILE_DEFS ${TC_COMPILE_DEFS}
)

ledger_unit_tests_add_test(
    NAME         test_transaction_check_ui_default_reply
    SOURCES      ${SDK_DIR}/app_features/transaction_check/src/transaction_check_ui.c
    MOCK_HEADERS ${SDK_DIR}/lib_standard_app/io.h
                 ${SDK_DIR}/lib_nbgl/include/nbgl_use_case.h
                 ${SDK_DIR}/lib_nbgl/include/nbgl_layout.h
                 ${SDK_DIR}/lib_nbgl/include/nbgl_obj.h
    INCLUDE_DIRS ${TC_INCLUDE_DIRS}
    COMPILE_DEFS ${TC_COMPILE_DEFS}
)
