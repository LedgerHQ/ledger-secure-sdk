#pragma once

#include <stddef.h>
#include <stdint.h>

/**
 * Generic TLV custom mutator for libFuzzer.
 *
 * Use-case agnostic: set current_tlv_fuzz_config from your harness
 * with the tag grammar of your TLV use case.
 * See fuzzing/README.md for integration details.
 */

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
