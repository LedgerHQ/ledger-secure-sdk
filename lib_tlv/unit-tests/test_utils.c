/*****************************************************************************
 *   (c) 2025 Ledger SAS.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *
 *****************************************************************************/
#include "test_utils.h"

#include <string.h>

/* DER-encode a uint32 value (used for both tags and lengths). */
static void der_encode(uint8_t *buffer, size_t *offset, uint32_t value)
{
    if (value < 0x80) {
        buffer[(*offset)++] = (uint8_t) value;
    }
    else if (value <= 0xFF) {
        buffer[(*offset)++] = 0x81;
        buffer[(*offset)++] = (uint8_t) value;
    }
    else if (value <= 0xFFFF) {
        buffer[(*offset)++] = 0x82;
        buffer[(*offset)++] = (uint8_t) (value >> 8);
        buffer[(*offset)++] = (uint8_t) value;
    }
    else {
        buffer[(*offset)++] = 0x84;
        buffer[(*offset)++] = (uint8_t) (value >> 24);
        buffer[(*offset)++] = (uint8_t) (value >> 16);
        buffer[(*offset)++] = (uint8_t) (value >> 8);
        buffer[(*offset)++] = (uint8_t) value;
    }
}

void append_tlv(uint8_t       *buffer,
                size_t        *offset,
                uint32_t       tag,
                const uint8_t *value,
                size_t         value_len)
{
    der_encode(buffer, offset, tag);
    der_encode(buffer, offset, (uint32_t) value_len);
    memcpy(&buffer[*offset], value, value_len);
    *offset += value_len;
}

void append_tlv_uint8(uint8_t *buffer, size_t *offset, uint32_t tag, uint8_t value)
{
    append_tlv(buffer, offset, tag, &value, 1);
}

void append_tlv_uint16(uint8_t *buffer, size_t *offset, uint32_t tag, uint16_t value)
{
    uint8_t bytes[2];
    bytes[0] = (value >> 8) & 0xFF;
    bytes[1] = value & 0xFF;
    append_tlv(buffer, offset, tag, bytes, sizeof(bytes));
}

void append_tlv_uint32(uint8_t *buffer, size_t *offset, uint32_t tag, uint32_t value)
{
    uint8_t bytes[4];
    bytes[0] = (value >> 24) & 0xFF;
    bytes[1] = (value >> 16) & 0xFF;
    bytes[2] = (value >> 8) & 0xFF;
    bytes[3] = value & 0xFF;
    append_tlv(buffer, offset, tag, bytes, sizeof(bytes));
}

void append_tlv_uint64(uint8_t *buffer, size_t *offset, uint32_t tag, uint64_t value)
{
    uint8_t bytes[8];
    bytes[0] = (value >> 56) & 0xFF;
    bytes[1] = (value >> 48) & 0xFF;
    bytes[2] = (value >> 40) & 0xFF;
    bytes[3] = (value >> 32) & 0xFF;
    bytes[4] = (value >> 24) & 0xFF;
    bytes[5] = (value >> 16) & 0xFF;
    bytes[6] = (value >> 8) & 0xFF;
    bytes[7] = value & 0xFF;
    append_tlv(buffer, offset, tag, bytes, sizeof(bytes));
}

void append_tlv_string(uint8_t *buffer, size_t *offset, uint32_t tag, const char *str)
{
    append_tlv(buffer, offset, tag, (const uint8_t *) str, strlen(str));
}
