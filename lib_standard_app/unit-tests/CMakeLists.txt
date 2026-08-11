project(unit_tests_lib_standard_app LANGUAGES C)

# LedgerUT and ledger_unit_tests_init() live in the root CMakeLists.txt.

set(STDAPP_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${UT_INCLUDE_DIRS_SDK}
)

set(STDAPP_COMPILE_DEFS
    ${UT_COMPILE_DEFS_SDK}
)

ledger_unit_tests_add_test(
    NAME         test_base58
    SOURCES      ${LEDGER_SDK_ROOT}/lib_standard_app/base58.c
    INCLUDE_DIRS ${STDAPP_INCLUDE_DIRS}
    COMPILE_DEFS ${STDAPP_COMPILE_DEFS}
)

ledger_unit_tests_add_test(
    NAME         test_bip32
    SOURCES      ${LEDGER_SDK_ROOT}/lib_standard_app/bip32.c
                 ${LEDGER_SDK_ROOT}/lib_standard_app/read.c
    INCLUDE_DIRS ${STDAPP_INCLUDE_DIRS}
    COMPILE_DEFS ${STDAPP_COMPILE_DEFS}
)

ledger_unit_tests_add_test(
    NAME         test_buffer
    SOURCES      ${LEDGER_SDK_ROOT}/lib_standard_app/buffer.c
                 ${LEDGER_SDK_ROOT}/lib_standard_app/bip32.c
                 ${LEDGER_SDK_ROOT}/lib_standard_app/varint.c
                 ${LEDGER_SDK_ROOT}/lib_standard_app/write.c
                 ${LEDGER_SDK_ROOT}/lib_standard_app/read.c
    INCLUDE_DIRS ${STDAPP_INCLUDE_DIRS}
    COMPILE_DEFS ${STDAPP_COMPILE_DEFS}
)

ledger_unit_tests_add_test(
    NAME         test_format
    SOURCES      ${LEDGER_SDK_ROOT}/lib_standard_app/format.c
    INCLUDE_DIRS ${STDAPP_INCLUDE_DIRS}
    COMPILE_DEFS ${STDAPP_COMPILE_DEFS}
)

ledger_unit_tests_add_test(
    NAME         test_write
    SOURCES      ${LEDGER_SDK_ROOT}/lib_standard_app/write.c
    INCLUDE_DIRS ${STDAPP_INCLUDE_DIRS}
    COMPILE_DEFS ${STDAPP_COMPILE_DEFS}
)

ledger_unit_tests_add_test(
    NAME         test_apdu_parser
    SOURCES      ${LEDGER_SDK_ROOT}/lib_standard_app/parser.c
    INCLUDE_DIRS ${STDAPP_INCLUDE_DIRS}
    COMPILE_DEFS ${STDAPP_COMPILE_DEFS}
)

# app_storage needs its own storage defines and mocks the NVM/CRC syscalls.
ledger_unit_tests_add_test(
    NAME            test_app_storage
    SOURCES         ${LEDGER_SDK_ROOT}/lib_standard_app/app_storage.c
    MOCK_HEADERS    ${LEDGER_SDK_ROOT}/include/os_nvm.h
                    ${LEDGER_SDK_ROOT}/lib_cxng/include/lcx_crc.h
    INCLUDE_DIRS    ${STDAPP_INCLUDE_DIRS}
    COMPILE_DEFS    ${STDAPP_COMPILE_DEFS}
                    UNIT_TESTING
                    WITH_STDIO
                    HAVE_APP_STORAGE
                    APP_STORAGE_SIZE=5555
                    HAVE_APP_STORAGE_PROP_SETTINGS
                    HAVE_APP_STORAGE_PROP_DATA
    COMPILE_OPTIONS -fsanitize=address,undefined
    LINK_OPTIONS    -fsanitize=address,undefined
)
