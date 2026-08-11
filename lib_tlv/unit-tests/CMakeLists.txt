project(unit_tests_lib_tlv LANGUAGES C)

# LedgerUT and ledger_unit_tests_init() live in the root CMakeLists.txt.

set(TLV_COMMON_SOURCES
    ${LEDGER_SDK_ROOT}/lib_tlv/tlv_library.c
)

set(TLV_USE_CASE_UTILS
    ${CMAKE_CURRENT_SOURCE_DIR}/test_utils.c
)

set(TLV_INCLUDE_DIRS
    ${UT_INCLUDE_DIRS_SDK}
)

set(TLV_COMPILE_DEFS
    ${UT_COMPILE_DEFS_SDK}
    UNIT_TESTING
    WITH_STDIO
    APP_STORAGE_SIZE=5555
    HAVE_APP_STORAGE
    HAVE_APP_STORAGE_PROP_SETTINGS
    HAVE_APP_STORAGE_PROP_DATA
)

set(TLV_COMPILE_OPTIONS -Wno-comment -fsanitize=address,undefined)
set(TLV_LINK_OPTIONS    -fsanitize=address,undefined)

ledger_unit_tests_add_test(
    NAME            test_tlv_helpers
    SOURCES         ${TLV_COMMON_SOURCES}
    INCLUDE_DIRS    ${TLV_INCLUDE_DIRS}
    COMPILE_DEFS    ${TLV_COMPILE_DEFS}
    COMPILE_OPTIONS ${TLV_COMPILE_OPTIONS} -Wno-pedantic
    LINK_OPTIONS    ${TLV_LINK_OPTIONS}
)

ledger_unit_tests_add_test(
    NAME            test_tlv_macros
    SOURCES         ${TLV_COMMON_SOURCES}
    INCLUDE_DIRS    ${TLV_INCLUDE_DIRS}
    COMPILE_DEFS    ${TLV_COMPILE_DEFS}
    COMPILE_OPTIONS ${TLV_COMPILE_OPTIONS} -Wno-pedantic
    LINK_OPTIONS    ${TLV_LINK_OPTIONS}
)

# dynamic_descriptor uses cx_sha256_init (inline → cx_sha256_init_no_throw),
# cx_hash_update/final, and CX_ASSERT
ledger_unit_tests_add_test(
    NAME            test_tlv_use_case_dynamic_descriptor
    SOURCES         ${TLV_COMMON_SOURCES}
                    ${TLV_USE_CASE_UTILS}
                    ${LEDGER_SDK_ROOT}/lib_tlv/use_cases/tlv_use_case_dynamic_descriptor.c
    MOCK_HEADERS    ${LEDGER_SDK_ROOT}/include/ledger_assert_internals.h
                    ${LEDGER_SDK_ROOT}/lib_cxng/include/lcx_sha256.h
                    ${LEDGER_SDK_ROOT}/lib_cxng/include/lcx_hash.h
                    ${LEDGER_SDK_ROOT}/lib_pki/ledger_pki.h
    INCLUDE_DIRS    ${TLV_INCLUDE_DIRS}
    COMPILE_DEFS    ${TLV_COMPILE_DEFS}
    COMPILE_OPTIONS ${TLV_COMPILE_OPTIONS} -Wno-nonnull-compare
    LINK_OPTIONS    ${TLV_LINK_OPTIONS}
)

# trusted_name uses all hash init functions + cx_hash_update/final + CX_ASSERT
# lcx_sha512.h is not self-contained: force-include lcx_hash.h to provide missing types
ledger_unit_tests_add_test(
    NAME            test_tlv_use_case_trusted_name
    SOURCES         ${TLV_COMMON_SOURCES}
                    ${TLV_USE_CASE_UTILS}
                    ${LEDGER_SDK_ROOT}/lib_tlv/use_cases/tlv_use_case_trusted_name.c
    MOCK_HEADERS    ${LEDGER_SDK_ROOT}/include/ledger_assert_internals.h
                    ${LEDGER_SDK_ROOT}/lib_cxng/include/lcx_sha256.h
                    ${LEDGER_SDK_ROOT}/lib_cxng/include/lcx_sha3.h
                    ${LEDGER_SDK_ROOT}/lib_cxng/include/lcx_ripemd160.h
                    ${LEDGER_SDK_ROOT}/lib_cxng/include/lcx_sha512.h
                    ${LEDGER_SDK_ROOT}/lib_cxng/include/lcx_hash.h
                    ${LEDGER_SDK_ROOT}/lib_pki/ledger_pki.h
    INCLUDE_DIRS    ${TLV_INCLUDE_DIRS}
    COMPILE_DEFS    ${TLV_COMPILE_DEFS}
    COMPILE_OPTIONS ${TLV_COMPILE_OPTIONS} -Wno-nonnull-compare
                    -include ${LEDGER_SDK_ROOT}/lib_cxng/include/lcx_hash.h
    LINK_OPTIONS    ${TLV_LINK_OPTIONS}
)

# transaction_check uses cx_sha256_init_no_throw, cx_hash_update/final,
# is_printable_string, os_pki_get_info, and strlcpy (libbsd)
ledger_unit_tests_add_test(
    NAME            test_tlv_use_case_transaction_check
    SOURCES         ${TLV_COMMON_SOURCES}
                    ${TLV_USE_CASE_UTILS}
                    ${LEDGER_SDK_ROOT}/lib_tlv/use_cases/tlv_use_case_transaction_check.c
    MOCK_HEADERS    ${LEDGER_SDK_ROOT}/include/ledger_assert_internals.h
                    ${LEDGER_SDK_ROOT}/lib_cxng/include/lcx_sha256.h
                    ${LEDGER_SDK_ROOT}/lib_cxng/include/lcx_hash.h
                    ${LEDGER_SDK_ROOT}/include/os_utils.h
                    ${LEDGER_SDK_ROOT}/include/os_pki.h
                    ${LEDGER_SDK_ROOT}/lib_pki/ledger_pki.h
    INCLUDE_DIRS    ${TLV_INCLUDE_DIRS}
                    /usr/include/bsd
    COMPILE_DEFS    ${TLV_COMPILE_DEFS}
                    LIBBSD_OVERLAY
    COMPILE_OPTIONS ${TLV_COMPILE_OPTIONS} -Wno-nonnull-compare
    LINK_OPTIONS    ${TLV_LINK_OPTIONS}
)
target_link_libraries(test_tlv_use_case_transaction_check PRIVATE bsd)
