/*****************************************************************************
 *   (c) 2026 Ledger SAS.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *
 *****************************************************************************/

#include <string.h>

#include "tlv_test_helpers.h"

/* Tags <= TLV_DER_SHORT_FORM_MAX: 1-byte form.
 * Tags >  TLV_DER_SHORT_FORM_MAX: 2-byte long form [TLV_DER_LONG_FORM_PREFIX, tag].
 * All payload field sizes used here are < 0x80, so lengths are always 1 byte. */
void tlv_append(uint8_t *buf, size_t *off, uint32_t tag, const uint8_t *val, uint8_t vlen)
{
    if (tag <= TLV_DER_SHORT_FORM_MAX) {
        buf[(*off)++] = (uint8_t) tag;
    }
    else {
        buf[(*off)++] = TLV_DER_LONG_FORM_PREFIX;
        buf[(*off)++] = (uint8_t) tag;
    }
    buf[(*off)++] = vlen;
    if (val && vlen > 0) {
        memcpy(buf + *off, val, vlen);
        *off += vlen;
    }
}

void tlv_u8(uint8_t *buf, size_t *off, uint32_t tag, uint8_t val)
{
    tlv_append(buf, off, tag, &val, 1);
}

/* m/44'/60'/0'/0/0 — 5 components, 21 bytes */
const uint8_t BIP32_ETH_PATH[21] = {
    0x05, 0x80, 0x00, 0x00, 0x2C, /* 44' */
    0x80, 0x00, 0x00, 0x3C,       /* 60' */
    0x80, 0x00, 0x00, 0x00,       /* 0'  */
    0x00, 0x00, 0x00, 0x00,       /* 0   */
    0x00, 0x00, 0x00, 0x00,       /* 0   */
};

const uint8_t DUMMY_IDENTIFIER[4] = {0x01, 0x02, 0x03, 0x04};
const uint8_t ZERO_32[32]         = {0};
const uint8_t ZERO_64[64]         = {0};
const uint8_t ETH_CHAIN_ID_1[8]   = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
