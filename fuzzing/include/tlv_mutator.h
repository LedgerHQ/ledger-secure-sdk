#pragma once
/**
 * @file tlv_mutator.h
 * @brief Optional grammar-aware TLV mutator for the tail region.
 *
 * Keeps mutated tails valid TLV so the fuzzer spends its budget on values
 * rather than on framing. Set @ref current_tlv_fuzz_config before calling
 * @ref tlv_custom_mutate(). Opt in per app via LEDGER_FUZZ_TLV_MUTATOR_SOURCE.
 */

#include <stddef.h>
#include <stdint.h>

/** @brief Length bounds the mutator keeps for one TLV tag. */
typedef struct {
    uint8_t tag;      ///< TLV tag byte.
    uint8_t min_len;  ///< Minimum value length to emit.
    uint8_t max_len;  ///< Maximum value length to emit.
} tlv_tag_info_t;

/** @brief The TLV grammar the mutator currently applies (one per command). */
typedef struct {
    const tlv_tag_info_t *tags_info;  ///< Array of allowed tags.
    size_t                num_tags;   ///< Number of entries in @ref tags_info.
} tlv_fuzz_config_t;

/** Active grammar; set it before calling @ref tlv_custom_mutate(). */
extern tlv_fuzz_config_t current_tlv_fuzz_config;

/** @brief Build a @ref tlv_fuzz_config_t from a @ref tlv_tag_info_t array. */
#define TLV_CFG(arr)                                                   \
    {                                                                  \
        .tags_info = (arr), .num_tags = sizeof(arr) / sizeof((arr)[0]) \
    }

/** @brief Mutate a TLV byte range in place, preserving valid framing. */
size_t tlv_custom_mutate(uint8_t *data, size_t size, size_t max_size, unsigned int seed);

/*
 * Indexed-grammar dispatch helper: picks the active command's grammar from
 * configs[] and mutates that command's TLV payload, falling back to the generic
 * mutator otherwise.
 *
 * The command index is read from the harness input's control bytes (see
 * fuzz_defs.h), the same bytes fuzz_harness_entry() uses to select the command,
 * so the grammar chosen here always matches the command that will run.
 */
#include "fuzz_mutator.h"

extern const size_t fuzz_n_commands;

static inline size_t fuzz_tlv_dispatch_mutate(uint8_t                 *data,
                                              size_t                   size,
                                              size_t                   max_size,
                                              unsigned int             seed,
                                              const tlv_fuzz_config_t *configs,
                                              size_t                   n_configs)
{
    const size_t ps = fuzz_prefix_size();

    /* Need the prefix plus control bytes plus a 2-byte payload length header. */
    if (ps == 0 || ps + FUZZ_CTRL_LEN + 2 >= max_size || size <= ps + FUZZ_CTRL_LEN + 2) {
        return fuzz_custom_mutator(data, size, max_size, seed);
    }

    uint8_t *input   = data + ps;
    size_t   cmd_idx = input[1] % fuzz_n_commands;

    if (cmd_idx >= n_configs || configs[cmd_idx].num_tags == 0 || (seed & 1U) != 0) {
        return fuzz_custom_mutator(data, size, max_size, seed);
    }

    current_tlv_fuzz_config = configs[cmd_idx];

    uint8_t *payload      = input + FUZZ_CTRL_LEN;
    size_t   payload_size = size - ps - FUZZ_CTRL_LEN;
    size_t   max_payload  = max_size - ps - FUZZ_CTRL_LEN;

    /* First two payload bytes carry the TLV length. */
    size_t tlv_size = tlv_custom_mutate(payload + 2, payload_size - 2, max_payload - 2, seed >> 2);

    payload[0] = (uint8_t) (tlv_size >> 8);
    payload[1] = (uint8_t) (tlv_size & 0xFF);

    return ps + FUZZ_CTRL_LEN + 2 + tlv_size;
}
