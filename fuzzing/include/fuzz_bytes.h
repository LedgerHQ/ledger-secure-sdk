#pragma once
/**
 * @file fuzz_bytes.h
 * @brief Little/big-endian read and write helpers for harnesses and builders.
 *
 * Protocol-specific encodings do not belong here -- Bitcoin's CompactSize
 * varint, for instance, stays in the Bitcoin app.
 */

#include <stddef.h>
#include <stdint.h>

/** @brief Write a 32-bit value little-endian. */
static inline void fuzz_write_u32_le(uint8_t *out, uint32_t v)
{
    out[0] = (uint8_t) (v & 0xFF);
    out[1] = (uint8_t) ((v >> 8) & 0xFF);
    out[2] = (uint8_t) ((v >> 16) & 0xFF);
    out[3] = (uint8_t) ((v >> 24) & 0xFF);
}

/** @brief Write a 64-bit value little-endian. */
static inline void fuzz_write_u64_le(uint8_t *out, uint64_t v)
{
    for (int i = 0; i < 8; i++) {
        out[i] = (uint8_t) ((v >> (8 * i)) & 0xFF);
    }
}

/** @brief Write a 32-bit value big-endian (BIP32 components, APDU counts). */
static inline void fuzz_write_u32_be(uint8_t *out, uint32_t v)
{
    out[0] = (uint8_t) (v >> 24);
    out[1] = (uint8_t) (v >> 16);
    out[2] = (uint8_t) (v >> 8);
    out[3] = (uint8_t) (v);
}

/** @brief Read a 16-bit little-endian value. */
static inline uint16_t fuzz_read_u16_le(const uint8_t *p)
{
    return (uint16_t) ((uint16_t) p[0] | ((uint16_t) p[1] << 8));
}

/** @brief Read a 32-bit little-endian value. */
static inline uint32_t fuzz_read_u32_le(const uint8_t *p)
{
    return (uint32_t) p[0] | ((uint32_t) p[1] << 8) | ((uint32_t) p[2] << 16)
           | ((uint32_t) p[3] << 24);
}

/** @brief Read a 64-bit little-endian value. */
static inline uint64_t fuzz_read_u64_le(const uint8_t *p)
{
    uint64_t v = 0;
    for (int i = 0; i < 8; i++) {
        v |= (uint64_t) p[i] << (8 * i);
    }
    return v;
}
