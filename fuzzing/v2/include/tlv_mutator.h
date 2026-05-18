#pragma once
/* Grammar-aware TLV custom mutator (tail-region only).
 *
 * Set current_tlv_fuzz_config to the active grammar before calling
 * tlv_custom_mutate(). Apps opt in by adding LEDGER_FUZZ_TLV_MUTATOR_SOURCE
 * to their target sources. */

#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint8_t tag;
    uint8_t min_len;
    uint8_t max_len;
} tlv_tag_info_t;

typedef struct {
    const tlv_tag_info_t *tags_info;
    size_t                num_tags;
} tlv_fuzz_config_t;

extern tlv_fuzz_config_t current_tlv_fuzz_config;

size_t tlv_custom_mutate(uint8_t *data, size_t size, size_t max_size, unsigned int seed);

/* Indexed-grammar dispatch helper for app harnesses.
 *
 * Picks the active command from the prefix layout, looks up its grammar in
 * configs[], and mutates the [4-byte APDU header | 2-byte length | TLV] tail.
 * Falls back to fuzz_custom_mutator() otherwise.
 *
 * Required before inclusion (provided by fuzz_mutator.h):
 * - FUZZ_PREFIX_SIZE_FALLBACK
 * - FUZZ_CTRL_OFF
 * - fuzz_lane_is_structured(data, prefix_size) */
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
