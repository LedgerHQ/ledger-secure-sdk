#pragma once

#include <stddef.h>   // size_t
#include <stdint.h>   // uint*_t
#include <stdbool.h>  // bool

/**
 * Hardening mask for BIP32 path elements
 */
#define BIP32_HARDENED_MASK 0x80000000u

/**
 * Maximum length of BIP32 path allowed.
 */
#define MAX_BIP32_PATH 10

typedef struct {
    uint8_t  length;
    uint32_t path[MAX_BIP32_PATH];
} path_bip32_t;

/**
 * Read BIP32 path from byte buffer.
 *
 * @param[in]  in Pointer to input byte buffer.
 * @param[in]  in_len Length of input byte buffer.
 * @param[out] out Pointer to output 32-bit integer buffer.
 * @param[in]  out_len Number of BIP32 paths read in the output buffer.
 *
 * @return true if success, false otherwise.
 *
 */
bool bip32_path_read(const uint8_t *in, size_t in_len, uint32_t *out, size_t out_len);

/**
 * Format BIP32 path as string.
 *
 * @param[in]  bip32_path Pointer to 32-bit integer input buffer.
 * @param[in]  bip32_path_len Maximum number of BIP32 paths in the input buffer.
 * @param[out] out Pointer to output string.
 * @param[in]  out_len Length of the output string.
 *
 * @return true if success, false otherwise.
 *
 */
bool bip32_path_format(const uint32_t *bip32_path,
                       size_t          bip32_path_len,
                       char           *out,
                       size_t          out_len);

/**
 * @brief Format a BIP32 path as a string.
 *
 * Convenience wrapper around bip32_path_format() for the path_bip32_t structure.
 *
 * @param[in]  bip32    The BIP32 path structure.
 * @param[out] out      The output buffer to store the formatted string.
 * @param[in]  out_len  The length of the output buffer.
 * @return true if the BIP32 path was successfully formatted, false otherwise.
 */
bool bip32_path_format_simple(path_bip32_t *bip32, char *out, size_t out_len);
