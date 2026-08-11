#pragma once
/**
 * @file fuzz_bip32.h
 * @brief Build a serialized BIP32 path from fuzz bytes.
 *
 * Output is the wire form every Ledger app's buffer_read_bip32_path() expects:
 * @c [len][component BE]... Configure the accepted purposes, coin type and depth
 * bounds through @ref fuzz_bip32_config_t; the app supplies the policy, the
 * framework supplies the encoding.
 */

#include <stddef.h>
#include <stdint.h>

#include "os_utils.h"  // U4BE_ENCODE

typedef struct {
    const uint32_t *purposes; /* e.g. {44, 49, 84, 86} */
    size_t          n_purposes;
    uint32_t        coin_type;   /* e.g. 0x80000000 | 1 for testnet */
    uint8_t         max_account; /* max account index (0-based) */
    uint8_t         max_depth;   /* max total path depth (3-5 typical) */
} fuzz_bip32_config_t;

// Build a BIP32 path into buf from ctrl bytes: [0] purpose, [1] account,
// [2] depth selector, [3..] extra child indices. Returns bytes written or 0.
static inline size_t fuzz_bip32_build(const fuzz_bip32_config_t *cfg,
                                      const uint8_t             *ctrl,
                                      size_t                     ctrl_len,
                                      uint8_t                   *buf,
                                      size_t                     buf_size)
{
    if (!cfg || cfg->n_purposes == 0 || !cfg->purposes) {
        return 0;
    }

    uint8_t min_depth = 3;
    uint8_t max_depth = cfg->max_depth;
    if (max_depth < min_depth) {
        max_depth = min_depth;
    }

    uint8_t depth = min_depth;
    if (ctrl_len > 2 && max_depth > min_depth) {
        depth = min_depth + (ctrl[2] % (max_depth - min_depth + 1));
    }

    size_t needed = 1 + (size_t) depth * 4;
    if (buf_size < needed) {
        return 0;
    }

    uint32_t purpose = cfg->purposes[0];
    if (ctrl_len > 0) {
        purpose = cfg->purposes[ctrl[0] % cfg->n_purposes];
    }

    uint32_t account = 0;
    if (ctrl_len > 1 && cfg->max_account > 0) {
        account = ctrl[1] % ((uint32_t) cfg->max_account + 1);
    }

    uint8_t *p = buf;
    *p++       = depth;

    U4BE_ENCODE(p, 0, 0x80000000UL | purpose);
    p += 4;

    if (depth > 1) {
        U4BE_ENCODE(p, 0, cfg->coin_type);
        p += 4;
    }

    if (depth > 2) {
        U4BE_ENCODE(p, 0, 0x80000000UL | account);
        p += 4;
    }

    for (uint8_t i = 3; i < depth; i++) {
        uint32_t child    = 0;
        size_t   ctrl_idx = 3 + (size_t) (i - 3);
        if (ctrl_idx < ctrl_len) {
            child = ctrl[ctrl_idx] % 2;
        }
        U4BE_ENCODE(p, 0, child);
        p += 4;
    }

    return (size_t) (p - buf);
}
