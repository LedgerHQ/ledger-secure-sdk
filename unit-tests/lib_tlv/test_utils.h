/*****************************************************************************
 *   (c) 2025 Ledger SAS.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *
 *****************************************************************************/
#pragma once

#include <stdint.h>
#include <stddef.h>

void append_tlv(uint8_t       *buffer,
                size_t        *offset,
                uint32_t       tag,
                const uint8_t *value,
                size_t         value_len);
void append_tlv_uint8(uint8_t *buffer, size_t *offset, uint32_t tag, uint8_t value);
void append_tlv_uint16(uint8_t *buffer, size_t *offset, uint32_t tag, uint16_t value);
void append_tlv_uint32(uint8_t *buffer, size_t *offset, uint32_t tag, uint32_t value);
void append_tlv_uint64(uint8_t *buffer, size_t *offset, uint32_t tag, uint64_t value);
void append_tlv_string(uint8_t *buffer, size_t *offset, uint32_t tag, const char *str);
