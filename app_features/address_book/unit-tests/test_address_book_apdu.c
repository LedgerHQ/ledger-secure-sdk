/*****************************************************************************
 *   (c) 2026 Ledger SAS.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *
 *****************************************************************************/

/**
 * @file test_address_book_apdu.c
 * @brief Unit tests for address_book.c: chunked reassembly + APDU dispatch.
 *
 * Sub-command handlers (register_identity, etc.) are stubbed here so that
 * only address_book.c (the dispatcher) is the code under test. The stubs
 * return SWO_SUCCESS unconditionally.
 *
 * Tests:
 *  - First chunk too short (< 2 bytes) → SWO_INCORRECT_DATA
 *  - Continuation chunk with no prior first chunk → SWO_INCORRECT_DATA
 *  - First chunk with total_len = 0 → SWO_INCORRECT_DATA
 *  - First chunk with total_len > buffer capacity → SWO_INCORRECT_DATA
 *  - Single chunk: full payload in one shot → dispatches stub → SWO_SUCCESS
 *  - Two-chunk reassembly → dispatches stub → SWO_SUCCESS
 *  - Continuation chunk exceeding declared total → SWO_INCORRECT_DATA
 *  - Unknown P1 → SWO_CONDITIONS_NOT_SATISFIED
 *  - P1=0x03 (edit_identifier) dispatched → SWO_SUCCESS
 *  - P1=0x04 (edit_scope) dispatched → SWO_SUCCESS
 *  - P1=0x20 (provide_contact) dispatched → SWO_SUCCESS
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "unity.h"
#include "Mockio.h"

#include "address_book.h"
#include "status_words.h"

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

/* ── Sub-command stubs ───────────────────────────────────────────────────── */
/* These match the declarations in identity.h / ledger_account.h.            */

bolos_err_t register_identity(uint8_t *b, size_t l)
{
    (void) b;
    (void) l;
    return SWO_SUCCESS;
}

bolos_err_t edit_contact_name(uint8_t *b, size_t l)
{
    (void) b;
    (void) l;
    return SWO_SUCCESS;
}

bolos_err_t edit_identifier(uint8_t *b, size_t l)
{
    (void) b;
    (void) l;
    return SWO_SUCCESS;
}

bolos_err_t edit_scope(uint8_t *b, size_t l)
{
    (void) b;
    (void) l;
    return SWO_SUCCESS;
}

bolos_err_t provide_contact(uint8_t *b, size_t l)
{
    (void) b;
    (void) l;
    return SWO_SUCCESS;
}

#ifdef HAVE_ADDRESS_BOOK_LEDGER_ACCOUNT
bolos_err_t register_ledger_account(uint8_t *b, size_t l)
{
    (void) b;
    (void) l;
    return SWO_SUCCESS;
}

bolos_err_t edit_ledger_account(uint8_t *b, size_t l)
{
    (void) b;
    (void) l;
    return SWO_SUCCESS;
}

bolos_err_t provide_ledger_account_contact(uint8_t *b, size_t l)
{
    (void) b;
    (void) l;
    return SWO_SUCCESS;
}
#endif /* HAVE_ADDRESS_BOOK_LEDGER_ACCOUNT */

/* ── Helpers ─────────────────────────────────────────────────────────────── */

/* Build a first-chunk buffer: 2-byte BE total length followed by payload. */
static size_t build_first_chunk(uint8_t       *out,
                                size_t         out_size,
                                uint16_t       total_len,
                                const uint8_t *payload,
                                size_t         payload_len)
{
    out[0] = (total_len >> 8) & 0xFF;
    out[1] = total_len & 0xFF;
    if (payload && payload_len > 0 && out_size >= 2 + payload_len) {
        memcpy(out + 2, payload, payload_len);
    }
    return 2 + payload_len;
}

/* ── Tests ───────────────────────────────────────────────────────────────── */

static void test_first_chunk_too_short(void)
{
    uint8_t buf[] = {0xAA}; /* 1 byte — header needs at least 2 */
    TEST_ASSERT_EQUAL_INT(SWO_INCORRECT_DATA, addr_book_handle_apdu(buf, sizeof(buf), 0x01, 0x00));
}

static void test_first_chunk_zero_total_length(void)
{
    uint8_t buf[] = {0x00, 0x00}; /* total_len = 0 → rejected */
    TEST_ASSERT_EQUAL_INT(SWO_INCORRECT_DATA, addr_book_handle_apdu(buf, sizeof(buf), 0x01, 0x00));
}

static void test_first_chunk_total_len_too_large(void)
{
    /* 0xFFFF always exceeds ADDRESS_BOOK_MAX_CHUNKED_PAYLOAD */
    uint8_t buf[] = {0xFF, 0xFF, 0x01};
    TEST_ASSERT_EQUAL_INT(SWO_INCORRECT_DATA, addr_book_handle_apdu(buf, sizeof(buf), 0x01, 0x00));
}

static void test_continuation_without_first(void)
{
    uint8_t buf[] = {0x01, 0x02, 0x03};
    /* P2=0x80 but no P2=0x00 chunk was received first */
    TEST_ASSERT_EQUAL_INT(SWO_INCORRECT_DATA, addr_book_handle_apdu(buf, sizeof(buf), 0x01, 0x80));
}

static void test_single_chunk_known_p1_register_identity(void)
{
    const uint8_t payload[] = {0x01, 0x02, 0x03};
    uint8_t       buf[5];
    size_t        len
        = build_first_chunk(buf, sizeof(buf), (uint16_t) sizeof(payload), payload, sizeof(payload));
    /* P1=0x01 → register_identity stub returns SWO_SUCCESS */
    TEST_ASSERT_EQUAL_INT(SWO_SUCCESS, addr_book_handle_apdu(buf, len, 0x01, 0x00));
}

static void test_single_chunk_known_p1_edit_contact_name(void)
{
    const uint8_t payload[] = {0xAA, 0xBB};
    uint8_t       buf[4];
    size_t        len
        = build_first_chunk(buf, sizeof(buf), (uint16_t) sizeof(payload), payload, sizeof(payload));
    TEST_ASSERT_EQUAL_INT(SWO_SUCCESS, addr_book_handle_apdu(buf, len, 0x02, 0x00));
}

static void test_single_chunk_known_p1_edit_identifier(void)
{
    const uint8_t payload[] = {0x01};
    uint8_t       buf[3];
    size_t        len
        = build_first_chunk(buf, sizeof(buf), (uint16_t) sizeof(payload), payload, sizeof(payload));
    TEST_ASSERT_EQUAL_INT(SWO_SUCCESS, addr_book_handle_apdu(buf, len, 0x03, 0x00));
}

static void test_single_chunk_known_p1_edit_scope(void)
{
    const uint8_t payload[] = {0x01};
    uint8_t       buf[3];
    size_t        len
        = build_first_chunk(buf, sizeof(buf), (uint16_t) sizeof(payload), payload, sizeof(payload));
    TEST_ASSERT_EQUAL_INT(SWO_SUCCESS, addr_book_handle_apdu(buf, len, 0x04, 0x00));
}

static void test_single_chunk_known_p1_provide_contact(void)
{
    const uint8_t payload[] = {0x01};
    uint8_t       buf[3];
    size_t        len
        = build_first_chunk(buf, sizeof(buf), (uint16_t) sizeof(payload), payload, sizeof(payload));
    TEST_ASSERT_EQUAL_INT(SWO_SUCCESS, addr_book_handle_apdu(buf, len, 0x20, 0x00));
}

static void test_single_chunk_unknown_p1(void)
{
    const uint8_t payload[] = {0xFF};
    uint8_t       buf[3];
    size_t        len
        = build_first_chunk(buf, sizeof(buf), (uint16_t) sizeof(payload), payload, sizeof(payload));
    /* P1=0xFF has no case in the switch → default → SWO_CONDITIONS_NOT_SATISFIED */
    TEST_ASSERT_EQUAL_INT(SWO_CONDITIONS_NOT_SATISFIED,
                          addr_book_handle_apdu(buf, len, 0xFF, 0x00));
}

static void test_two_chunk_reassembly(void)
{
    const uint8_t part1[] = {0x01, 0x02, 0x03};
    const uint8_t part2[] = {0x04, 0x05, 0x06};
    uint16_t      total   = sizeof(part1) + sizeof(part2);

    uint8_t first[8];
    size_t  first_len = build_first_chunk(first, sizeof(first), total, part1, sizeof(part1));

    /* First chunk → REASSEMBLY_PENDING → SWO_SUCCESS */
    TEST_ASSERT_EQUAL_INT(SWO_SUCCESS, addr_book_handle_apdu(first, first_len, 0x01, 0x00));

    /* Continuation chunk → REASSEMBLY_COMPLETE → register_identity stub → SWO_SUCCESS */
    uint8_t cont[sizeof(part2)];
    memcpy(cont, part2, sizeof(part2));
    TEST_ASSERT_EQUAL_INT(SWO_SUCCESS, addr_book_handle_apdu(cont, sizeof(cont), 0x01, 0x80));
}

static void test_continuation_overflow(void)
{
    /* Declare total=3 bytes; send 3 bytes in first chunk (complete already). */
    /* Then try to send 1 more byte as continuation → overflow. */
    const uint8_t payload[] = {0x01, 0x02, 0x03};
    uint8_t       first[5];
    size_t        first_len = build_first_chunk(
        first, sizeof(first), (uint16_t) sizeof(payload), payload, sizeof(payload));

    /* Single-chunk: complete in one shot → dispatches stub → SWO_SUCCESS */
    TEST_ASSERT_EQUAL_INT(SWO_SUCCESS, addr_book_handle_apdu(first, first_len, 0x01, 0x00));

    /* Now try an orphan continuation (s_chunk_total was reset to 0 after completion). */
    uint8_t extra[] = {0xDE};
    TEST_ASSERT_EQUAL_INT(SWO_INCORRECT_DATA,
                          addr_book_handle_apdu(extra, sizeof(extra), 0x01, 0x80));
}

static void test_continuation_exceeds_total(void)
{
    /* Declare total=2, send 1 byte in first chunk (pending), then send 4 bytes → overflow */
    uint8_t first[3];
    first[0] = 0x00;
    first[1] = 0x02; /* total = 2 */
    first[2] = 0xAA; /* 1 byte of payload */
    TEST_ASSERT_EQUAL_INT(SWO_SUCCESS, addr_book_handle_apdu(first, sizeof(first), 0x01, 0x00));

    /* Continuation: 4 bytes but only 1 byte remaining → overflow */
    uint8_t cont[] = {0xBB, 0xCC, 0xDD, 0xEE};
    TEST_ASSERT_EQUAL_INT(SWO_INCORRECT_DATA,
                          addr_book_handle_apdu(cont, sizeof(cont), 0x01, 0x80));
}

/* ── Test runner ─────────────────────────────────────────────────────────── */

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_first_chunk_too_short);
    RUN_TEST(test_first_chunk_zero_total_length);
    RUN_TEST(test_first_chunk_total_len_too_large);
    RUN_TEST(test_continuation_without_first);
    RUN_TEST(test_single_chunk_known_p1_register_identity);
    RUN_TEST(test_single_chunk_known_p1_edit_contact_name);
    RUN_TEST(test_single_chunk_known_p1_edit_identifier);
    RUN_TEST(test_single_chunk_known_p1_edit_scope);
    RUN_TEST(test_single_chunk_known_p1_provide_contact);
    RUN_TEST(test_single_chunk_unknown_p1);
    RUN_TEST(test_two_chunk_reassembly);
    RUN_TEST(test_continuation_overflow);
    RUN_TEST(test_continuation_exceeds_total);

    return UNITY_END();
}
