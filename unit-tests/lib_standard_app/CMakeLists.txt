cmake_minimum_required(VERSION 3.10)
project(unit_tests_lib_standard_app LANGUAGES C)

set(SDK_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../..)
include(${SDK_DIR}/cmake/LedgerUT.cmake)

ledger_unit_tests_init()

set(STDAPP_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${UT_INCLUDE_DIRS_SDK}
)

set(STDAPP_COMPILE_DEFS
    ${UT_COMPILE_DEFS_SDK}
)

ledger_unit_tests_add_test(
    NAME         test_base58
    SOURCES      ${SDK_DIR}/lib_standard_app/base58.c
    INCLUDE_DIRS ${STDAPP_INCLUDE_DIRS}
    COMPILE_DEFS ${STDAPP_COMPILE_DEFS}
)

ledger_unit_tests_add_test(
    NAME         test_bip32
    SOURCES      ${SDK_DIR}/lib_standard_app/bip32.c
                 ${SDK_DIR}/lib_standard_app/read.c
    INCLUDE_DIRS ${STDAPP_INCLUDE_DIRS}
    COMPILE_DEFS ${STDAPP_COMPILE_DEFS}
)

ledger_unit_tests_add_test(
    NAME         test_buffer
    SOURCES      ${SDK_DIR}/lib_standard_app/buffer.c
                 ${SDK_DIR}/lib_standard_app/bip32.c
                 ${SDK_DIR}/lib_standard_app/varint.c
                 ${SDK_DIR}/lib_standard_app/write.c
                 ${SDK_DIR}/lib_standard_app/read.c
    INCLUDE_DIRS ${STDAPP_INCLUDE_DIRS}
    COMPILE_DEFS ${STDAPP_COMPILE_DEFS}
)

ledger_unit_tests_add_test(
    NAME         test_format
    SOURCES      ${SDK_DIR}/lib_standard_app/format.c
    INCLUDE_DIRS ${STDAPP_INCLUDE_DIRS}
    COMPILE_DEFS ${STDAPP_COMPILE_DEFS}
)

ledger_unit_tests_add_test(
    NAME         test_write
    SOURCES      ${SDK_DIR}/lib_standard_app/write.c
    INCLUDE_DIRS ${STDAPP_INCLUDE_DIRS}
    COMPILE_DEFS ${STDAPP_COMPILE_DEFS}
)

ledger_unit_tests_add_test(
    NAME         test_apdu_parser
    SOURCES      ${SDK_DIR}/lib_standard_app/parser.c
    INCLUDE_DIRS ${STDAPP_INCLUDE_DIRS}
    COMPILE_DEFS ${STDAPP_COMPILE_DEFS}
)
