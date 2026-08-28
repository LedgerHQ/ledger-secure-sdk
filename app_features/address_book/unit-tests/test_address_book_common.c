/*****************************************************************************
 *   (c) 2026 Ledger SAS.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *
 *****************************************************************************/

/**
 * @file test_address_book_common.c
 * @brief Unit tests for address_book_common.c TLV field handlers.
 *
 * Tests:
 *  - address_book_handle_chain_id()
 *  - address_book_handle_blockchain_family()
 *  - address_book_handle_printable_string()
 *  - address_book_handle_derivation_path()
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "unity.h"
#include "Mockio.h"

bool is_printable_string(const char *str, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        if (!isprint((unsigned char) str[i])) {
            return false;
        }
    }
    return true;
}

#include "address_book_common.h"
#include "tlv_library.h"
#include "bip32.h"

void setUp(void)
{
    Mockio_Init();
    io_send_response_buffers_IgnoreAndReturn(0);
}

void tearDown(void)
{
    Mockio_Verify();
    Mockio_Destroy();
}

/* ── Helpers ─────────────────────────────────────────────────────────────── */

static tlv_data_t make_tlv(const uint8_t *ptr, size_t size)
{
    tlv_data_t d = {0};
    d.value.ptr  = (uint8_t *) (uintptr_t) ptr; /* data is read-only in handlers */
    d.value.size = size;
    return d;
}

/* ── address_book_handle_chain_id ────────────────────────────────────────── */

static void test_chain_id_ethereum_mainnet(void)
{
    const uint8_t bytes[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
    tlv_data_t    data    = make_tlv(bytes, sizeof(bytes));
    uint64_t      chain_id;
    TEST_ASSERT_TRUE(address_book_handle_chain_id(&data, &chain_id));
    TEST_ASSERT_EQUAL_UINT64(1, chain_id);
}

static void test_chain_id_polygon(void)
{
    /* 137 = 0x89 */
    const uint8_t bytes[] = {0x89};
    tlv_data_t    data    = make_tlv(bytes, sizeof(bytes));
    uint64_t      chain_id;
    TEST_ASSERT_TRUE(address_book_handle_chain_id(&data, &chain_id));
    TEST_ASSERT_EQUAL_UINT64(137, chain_id);
}

static void test_chain_id_max(void)
{
    const uint8_t bytes[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    tlv_data_t    data    = make_tlv(bytes, sizeof(bytes));
    uint64_t      chain_id;
    TEST_ASSERT_TRUE(address_book_handle_chain_id(&data, &chain_id));
    TEST_ASSERT_EQUAL_UINT64(UINT64_MAX, chain_id);
}

static void test_chain_id_empty_value(void)
{
    tlv_data_t data = make_tlv(NULL, 0);
    uint64_t   chain_id;
    TEST_ASSERT_FALSE(address_book_handle_chain_id(&data, &chain_id));
}

static void test_chain_id_too_long(void)
{
    const uint8_t bytes[9] = {0};
    tlv_data_t    data     = make_tlv(bytes, sizeof(bytes));
    uint64_t      chain_id;
    TEST_ASSERT_FALSE(address_book_handle_chain_id(&data, &chain_id));
}

/* ── address_book_handle_blockchain_family ───────────────────────────────── */

static void test_blockchain_family_bitcoin(void)
{
    const uint8_t       byte = (uint8_t) FAMILY_BITCOIN;
    tlv_data_t          data = make_tlv(&byte, 1);
    blockchain_family_e family;
    TEST_ASSERT_TRUE(address_book_handle_blockchain_family(&data, &family));
    TEST_ASSERT_EQUAL_INT(FAMILY_BITCOIN, family);
}

static void test_blockchain_family_ethereum(void)
{
    const uint8_t       byte = (uint8_t) FAMILY_ETHEREUM;
    tlv_data_t          data = make_tlv(&byte, 1);
    blockchain_family_e family;
    TEST_ASSERT_TRUE(address_book_handle_blockchain_family(&data, &family));
    TEST_ASSERT_EQUAL_INT(FAMILY_ETHEREUM, family);
}

static void test_blockchain_family_all_valid(void)
{
    for (uint8_t i = 0; i < (uint8_t) FAMILY_COUNT; i++) {
        tlv_data_t          data = make_tlv(&i, 1);
        blockchain_family_e family;
        TEST_ASSERT_TRUE(address_book_handle_blockchain_family(&data, &family));
        TEST_ASSERT_EQUAL_INT((blockchain_family_e) i, family);
    }
}

static void test_blockchain_family_out_of_range(void)
{
    const uint8_t       byte = (uint8_t) FAMILY_COUNT;
    tlv_data_t          data = make_tlv(&byte, 1);
    blockchain_family_e family;
    TEST_ASSERT_FALSE(address_book_handle_blockchain_family(&data, &family));
}

static void test_blockchain_family_out_of_range_max(void)
{
    const uint8_t       byte = 0xFF;
    tlv_data_t          data = make_tlv(&byte, 1);
    blockchain_family_e family;
    TEST_ASSERT_FALSE(address_book_handle_blockchain_family(&data, &family));
}

static void test_blockchain_family_wrong_length(void)
{
    blockchain_family_e family;
    /* get_uint8_t_from_tlv_data rejects values > UINT8_MAX, but size=2 means
     * get_uint64_t_from_tlv_data reads a two-byte value; 0x0000 ≤ UINT8_MAX,
     * so it succeeds. Patch it with a value that actually overflows uint8_t. */
    const uint8_t overflow[2] = {0x01, 0x00}; /* 256 > UINT8_MAX */
    tlv_data_t    data        = make_tlv(overflow, sizeof(overflow));
    TEST_ASSERT_FALSE(address_book_handle_blockchain_family(&data, &family));
}

/* ── address_book_handle_printable_string ────────────────────────────────── */

static void test_printable_string_valid(void)
{
    const char *str                      = "Alice";
    tlv_data_t  data                     = make_tlv((const uint8_t *) str, strlen(str));
    char        out[CONTACT_NAME_LENGTH] = {0};
    TEST_ASSERT_TRUE(address_book_handle_printable_string(&data, out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("Alice", out);
}

static void test_printable_string_max_length(void)
{
    /* 32 chars = CONTACT_NAME_LENGTH - 1 (the max printable length) */
    const char str32[32]                = "ABCDEFGHIJKLMNOPQRSTUVWXYZ012345";
    tlv_data_t data                     = make_tlv((const uint8_t *) str32, sizeof(str32));
    char       out[CONTACT_NAME_LENGTH] = {0};
    TEST_ASSERT_TRUE(address_book_handle_printable_string(&data, out, sizeof(out)));
    TEST_ASSERT_EQUAL_MEMORY(str32, out, sizeof(str32));
}

static void test_printable_string_too_long(void)
{
    /* 33 chars would require a 34-byte buffer — one more than CONTACT_NAME_LENGTH */
    uint8_t bytes[33];
    memset(bytes, 'A', sizeof(bytes));
    tlv_data_t data                     = make_tlv(bytes, sizeof(bytes));
    char       out[CONTACT_NAME_LENGTH] = {0};
    TEST_ASSERT_FALSE(address_book_handle_printable_string(&data, out, sizeof(out)));
}

static void test_printable_string_empty_rejected(void)
{
    /* min_len=1 → empty string is rejected */
    tlv_data_t data                     = make_tlv((const uint8_t *) "", 0);
    char       out[CONTACT_NAME_LENGTH] = {0};
    TEST_ASSERT_FALSE(address_book_handle_printable_string(&data, out, sizeof(out)));
}

static void test_printable_string_non_printable(void)
{
    const uint8_t bytes[]                  = {'H', 'e', '\x01', 'l', 'o'};
    tlv_data_t    data                     = make_tlv(bytes, sizeof(bytes));
    char          out[CONTACT_NAME_LENGTH] = {0};
    TEST_ASSERT_FALSE(address_book_handle_printable_string(&data, out, sizeof(out)));
}

static void test_printable_string_tab_rejected(void)
{
    const uint8_t bytes[]                  = {'A', '\t', 'B'};
    tlv_data_t    data                     = make_tlv(bytes, sizeof(bytes));
    char          out[CONTACT_NAME_LENGTH] = {0};
    /* '\t' (0x09) is not printable per isprint() */
    TEST_ASSERT_FALSE(address_book_handle_printable_string(&data, out, sizeof(out)));
}

static void test_printable_string_embedded_null_rejected(void)
{
    const uint8_t bytes[]                  = {'A', '\0', 'B'};
    tlv_data_t    data                     = make_tlv(bytes, sizeof(bytes));
    char          out[CONTACT_NAME_LENGTH] = {0};
    TEST_ASSERT_FALSE(address_book_handle_printable_string(&data, out, sizeof(out)));
}

/* ── address_book_handle_derivation_path ─────────────────────────────────── */

/* Build the BIP32 path wire format: [length_byte, comp0_be32, ..., compN_be32] */
static size_t build_bip32_buffer(uint8_t *buf, size_t buf_size, const uint32_t *comps, uint8_t n)
{
    if (buf_size < (size_t) (1 + n * 4)) {
        return 0;
    }
    buf[0] = n;
    for (uint8_t i = 0; i < n; i++) {
        buf[1 + i * 4]     = (comps[i] >> 24) & 0xFF;
        buf[1 + i * 4 + 1] = (comps[i] >> 16) & 0xFF;
        buf[1 + i * 4 + 2] = (comps[i] >> 8) & 0xFF;
        buf[1 + i * 4 + 3] = comps[i] & 0xFF;
    }
    return 1 + n * 4;
}

static void test_derivation_path_valid_5_components(void)
{
    /* m/44'/60'/0'/0/0 */
    const uint32_t comps[] = {0x8000002C, 0x8000003C, 0x80000000, 0, 0};
    uint8_t        buf[41] = {0};
    size_t         len     = build_bip32_buffer(buf, sizeof(buf), comps, 5);
    tlv_data_t     data    = make_tlv(buf, len);
    path_bip32_t   path    = {0};
    TEST_ASSERT_TRUE(address_book_handle_derivation_path(&data, &path));
    TEST_ASSERT_EQUAL_INT(5, path.length);
    TEST_ASSERT_EQUAL_HEX32(0x8000002C, path.path[0]);
    TEST_ASSERT_EQUAL_HEX32(0x8000003C, path.path[1]);
    TEST_ASSERT_EQUAL_HEX32(0x80000000, path.path[2]);
    TEST_ASSERT_EQUAL_HEX32(0x00000000, path.path[3]);
    TEST_ASSERT_EQUAL_HEX32(0x00000000, path.path[4]);
}

static void test_derivation_path_zero_components(void)
{
    /* buffer_get_path_bip32 rejects out_len==0 — so 0-component paths are invalid. */
    const uint8_t buf[] = {0x00}; /* 0 components */
    tlv_data_t    data  = make_tlv(buf, sizeof(buf));
    path_bip32_t  path  = {0};
    TEST_ASSERT_FALSE(address_book_handle_derivation_path(&data, &path));
}

static void test_derivation_path_empty_tlv_rejected(void)
{
    /* TLV value length 0 < min_size 1 → rejected by get_buffer_from_tlv_data */
    tlv_data_t   data = make_tlv(NULL, 0);
    path_bip32_t path = {0};
    TEST_ASSERT_FALSE(address_book_handle_derivation_path(&data, &path));
}

static void test_derivation_path_too_many_components(void)
{
    /* 11 components → 1 + 11*4 = 45 bytes > max_size=41 */
    uint8_t buf[45] = {0};
    buf[0]          = 11;
    /* fill component bytes with zeros */
    tlv_data_t   data = make_tlv(buf, sizeof(buf));
    path_bip32_t path = {0};
    TEST_ASSERT_FALSE(address_book_handle_derivation_path(&data, &path));
}

static void test_derivation_path_truncated_rejected(void)
{
    /* Claims 3 components but only provides 1 component worth of data */
    uint8_t      buf[5] = {0x03, 0x80, 0x00, 0x00, 0x00};
    tlv_data_t   data   = make_tlv(buf, sizeof(buf));
    path_bip32_t path   = {0};
    TEST_ASSERT_FALSE(address_book_handle_derivation_path(&data, &path));
}

/* ── Test runner ─────────────────────────────────────────────────────────── */

int main(void)
{
    UNITY_BEGIN();

    /* chain_id */
    RUN_TEST(test_chain_id_ethereum_mainnet);
    RUN_TEST(test_chain_id_polygon);
    RUN_TEST(test_chain_id_max);
    RUN_TEST(test_chain_id_empty_value);
    RUN_TEST(test_chain_id_too_long);

    /* blockchain_family */
    RUN_TEST(test_blockchain_family_bitcoin);
    RUN_TEST(test_blockchain_family_ethereum);
    RUN_TEST(test_blockchain_family_all_valid);
    RUN_TEST(test_blockchain_family_out_of_range);
    RUN_TEST(test_blockchain_family_out_of_range_max);
    RUN_TEST(test_blockchain_family_wrong_length);

    /* printable_string */
    RUN_TEST(test_printable_string_valid);
    RUN_TEST(test_printable_string_max_length);
    RUN_TEST(test_printable_string_too_long);
    RUN_TEST(test_printable_string_empty_rejected);
    RUN_TEST(test_printable_string_non_printable);
    RUN_TEST(test_printable_string_tab_rejected);
    RUN_TEST(test_printable_string_embedded_null_rejected);

    /* derivation_path */
    RUN_TEST(test_derivation_path_valid_5_components);
    RUN_TEST(test_derivation_path_zero_components);
    RUN_TEST(test_derivation_path_empty_tlv_rejected);
    RUN_TEST(test_derivation_path_too_many_components);
    RUN_TEST(test_derivation_path_truncated_rejected);

    return UNITY_END();
}
