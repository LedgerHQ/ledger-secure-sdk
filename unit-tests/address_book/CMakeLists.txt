cmake_minimum_required(VERSION 3.10)
project(unit_tests_address_book LANGUAGES C)

set(SDK_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../..)
include(${SDK_DIR}/cmake/LedgerUT.cmake)

ledger_unit_tests_init()

set(AB_SOURCES_COMMON
    ${SDK_DIR}/lib_tlv/tlv_library.c
    ${SDK_DIR}/lib_standard_app/bip32.c
    ${SDK_DIR}/lib_standard_app/buffer.c
    ${SDK_DIR}/lib_standard_app/read.c
    ${SDK_DIR}/lib_standard_app/write.c
    ${SDK_DIR}/lib_standard_app/varint.c
    ${SDK_DIR}/app_features/address_book/src/address_book_common.c
)

set(AB_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${SDK_DIR}/app_features/address_book/include
    ${UT_INCLUDE_DIRS_SDK}
)

set(AB_COMPILE_DEFS
    ${UT_COMPILE_DEFS_SDK}
    HAVE_ADDRESS_BOOK
    HAVE_ADDRESS_BOOK_LEDGER_ACCOUNT
    APPNAME="UnitTests"
    WITH_STDIO
)

ledger_unit_tests_add_test(
    NAME         test_address_book_common
    SOURCES      ${AB_SOURCES_COMMON}
    MOCK_HEADERS ${SDK_DIR}/lib_standard_app/io.h
                 ${SDK_DIR}/lib_nbgl/include/nbgl_use_case.h
    INCLUDE_DIRS ${AB_INCLUDE_DIRS}
    COMPILE_DEFS ${AB_COMPILE_DEFS}
)

# provide_contact split: one binary per compilation unit under test
ledger_unit_tests_add_test(
    NAME            test_identity_provide_contact
    SOURCES         ${AB_SOURCES_COMMON}
                    tlv_test_helpers.c
                    ${SDK_DIR}/app_features/address_book/src/address_book_crypto.c
                    ${SDK_DIR}/app_features/address_book/src/identity_provide_contact.c
    MOCK_HEADERS    ${SDK_DIR}/lib_standard_app/io.h
                    ${SDK_DIR}/include/ledger_assert_internals.h
                    ${SDK_DIR}/include/os_address_book.h
                    ${SDK_DIR}/include/os_utils.h
                    ${SDK_DIR}/include/os_pic.h
                    ${SDK_DIR}/lib_nbgl/include/nbgl_use_case.h
                    ${SDK_DIR}/lib_cxng/include/lcx_rng.h
    INCLUDE_DIRS    ${AB_INCLUDE_DIRS}
    COMPILE_DEFS    ${AB_COMPILE_DEFS}
    COMPILE_OPTIONS -Wno-array-parameter
)

ledger_unit_tests_add_test(
    NAME            test_ledger_account_provide_contact
    SOURCES         ${AB_SOURCES_COMMON}
                    tlv_test_helpers.c
                    ${SDK_DIR}/app_features/address_book/src/address_book_crypto.c
                    ${SDK_DIR}/app_features/address_book/src/ledger_account_provide_contact.c
    MOCK_HEADERS    ${SDK_DIR}/lib_standard_app/io.h
                    ${SDK_DIR}/include/ledger_assert_internals.h
                    ${SDK_DIR}/include/os_address_book.h
                    ${SDK_DIR}/include/os_utils.h
                    ${SDK_DIR}/include/os_pic.h
                    ${SDK_DIR}/lib_nbgl/include/nbgl_use_case.h
                    ${SDK_DIR}/lib_cxng/include/lcx_rng.h
    INCLUDE_DIRS    ${AB_INCLUDE_DIRS}
    COMPILE_DEFS    ${AB_COMPILE_DEFS}
    COMPILE_OPTIONS -Wno-array-parameter
)

# ui_flows split: identity flows vs ledger-account flows
ledger_unit_tests_add_test(
    NAME            test_identity_ui_flows
    SOURCES         ${AB_SOURCES_COMMON}
                    tlv_test_helpers.c
                    ${SDK_DIR}/app_features/address_book/src/address_book_crypto.c
                    ${SDK_DIR}/app_features/address_book/src/identity_register.c
                    ${SDK_DIR}/app_features/address_book/src/identity_edit_contact_name.c
                    ${SDK_DIR}/app_features/address_book/src/identity_edit_identifier.c
                    ${SDK_DIR}/app_features/address_book/src/identity_edit_scope.c
    MOCK_HEADERS    ${SDK_DIR}/lib_standard_app/io.h
                    ${SDK_DIR}/include/ledger_assert_internals.h
                    ${SDK_DIR}/include/os_address_book.h
                    ${SDK_DIR}/include/os_utils.h
                    ${SDK_DIR}/include/os_pic.h
                    ${SDK_DIR}/lib_cxng/include/lcx_rng.h
    INCLUDE_DIRS    ${AB_INCLUDE_DIRS}
    COMPILE_DEFS    ${AB_COMPILE_DEFS}
    COMPILE_OPTIONS -Wno-array-parameter
)

ledger_unit_tests_add_test(
    NAME            test_ledger_account_ui_flows
    SOURCES         ${AB_SOURCES_COMMON}
                    tlv_test_helpers.c
                    ${SDK_DIR}/app_features/address_book/src/address_book_crypto.c
                    ${SDK_DIR}/app_features/address_book/src/ledger_account_register.c
                    ${SDK_DIR}/app_features/address_book/src/ledger_account_edit.c
    MOCK_HEADERS    ${SDK_DIR}/lib_standard_app/io.h
                    ${SDK_DIR}/include/ledger_assert_internals.h
                    ${SDK_DIR}/include/os_address_book.h
                    ${SDK_DIR}/include/os_utils.h
                    ${SDK_DIR}/include/os_pic.h
                    ${SDK_DIR}/lib_cxng/include/lcx_rng.h
    INCLUDE_DIRS    ${AB_INCLUDE_DIRS}
    COMPILE_DEFS    ${AB_COMPILE_DEFS}
    COMPILE_OPTIONS -Wno-array-parameter
)

ledger_unit_tests_add_test(
    NAME         test_address_book_apdu
    SOURCES      ${AB_SOURCES_COMMON}
                 ${SDK_DIR}/app_features/address_book/src/address_book.c
    MOCK_HEADERS ${SDK_DIR}/lib_standard_app/io.h
                 ${SDK_DIR}/include/os_utils.h
                 ${SDK_DIR}/lib_nbgl/include/nbgl_use_case.h
    INCLUDE_DIRS ${AB_INCLUDE_DIRS}
    COMPILE_DEFS ${AB_COMPILE_DEFS}
)
