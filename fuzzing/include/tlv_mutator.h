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

/** @brief Mutate a TLV byte range in place, preserving valid framing. */
size_t tlv_custom_mutate(uint8_t *data, size_t size, size_t max_size, unsigned int seed);

/* Indexed-grammar dispatch helper: picks the active command's grammar from configs[] and mutates
 * the TLV tail, else falls back to fuzz_custom_mutator(). */
#if defined(FUZZ_PREFIX_SIZE_FALLBACK) && defined(FUZZ_CTRL_OFF) && defined(fuzz_lane_is_structured)

extern const size_t absolution_globals_size __attribute__((weak));
extern const size_t fuzz_n_commands;
size_t fuzz_custom_mutator(uint8_t *data, size_t size, size_t max_size, unsigned int seed);

static inline size_t fuzz_tlv_dispatch_mutate(uint8_t                 *data,
                                              size_t                   size,
                                              size_t                   max_size,
                                              unsigned int             seed,
                                              const tlv_fuzz_config_t *configs,
                                              size_t                   n_configs)
{
    size_t ps = FUZZ_PREFIX_SIZE_FALLBACK;
    if (&absolution_globals_size != NULL && absolution_globals_size != 0) {
        ps = absolution_globals_size;
    }

    if (ps == 0 || ps + 6 >= max_size || size <= ps + 6) {
        return fuzz_custom_mutator(data, size, max_size, seed);
    }

    uint8_t cmd_byte = fuzz_lane_is_structured(data, ps) ? data[FUZZ_CTRL_OFF + 1] : data[ps + 1];
    size_t  cmd_idx  = cmd_byte % fuzz_n_commands;

    if (cmd_idx >= n_configs || configs[cmd_idx].num_tags == 0 || (seed & 1U) != 0) {
        return fuzz_custom_mutator(data, size, max_size, seed);
    }

    current_tlv_fuzz_config = configs[cmd_idx];

    uint8_t *payload      = data + ps + 4;
    size_t   payload_size = size - ps - 4;
    size_t   max_payload  = max_size - ps - 4;

    if (payload_size <= 2 || max_payload <= 2) {
        return fuzz_custom_mutator(data, size, max_size, seed);
    }

    uint8_t *tlv_data = payload + 2;
    size_t   tlv_size = tlv_custom_mutate(tlv_data, payload_size - 2, max_payload - 2, seed >> 2);

    payload[0] = (uint8_t) (tlv_size >> 8);
    payload[1] = (uint8_t) (tlv_size & 0xFF);

    return ps + 4 + 2 + tlv_size;
}

#endif
