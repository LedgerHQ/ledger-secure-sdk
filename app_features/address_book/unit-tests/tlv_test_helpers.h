/*****************************************************************************
 *   (c) 2026 Ledger SAS.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *
 *****************************************************************************/

/**
 * @file tlv_test_helpers.h
 * @brief Shared TLV builder helpers and test constants for address-book unit tests.
 */

#ifndef TLV_TEST_HELPERS_H
#define TLV_TEST_HELPERS_H

#include <stdint.h>
#include <stddef.h>

/* DER encoding constants (mirrors tlv_library.c internals) */
#define TLV_DER_LONG_FORM_PREFIX 0x81u /* tag >= 0x80: two-byte form [0x81, tag] */
#define TLV_DER_SHORT_FORM_MAX   0x7Fu /* tags <= this fit in one byte            */

void tlv_append(uint8_t *buf, size_t *off, uint32_t tag, const uint8_t *val, uint8_t vlen);
void tlv_u8(uint8_t *buf, size_t *off, uint32_t tag, uint8_t val);

/* m/44'/60'/0'/0/0 — 5 components, 21 bytes */
extern const uint8_t BIP32_ETH_PATH[21];

/* Arbitrary 4-byte identifier */
extern const uint8_t DUMMY_IDENTIFIER[4];
extern const uint8_t ZERO_32[32]; /* HMAC values   */
extern const uint8_t ZERO_64[64]; /* GROUP_HANDLE  */
extern const uint8_t ETH_CHAIN_ID_1[8];

#endif /* TLV_TEST_HELPERS_H */
