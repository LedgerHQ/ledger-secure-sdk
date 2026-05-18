/*
 * Grammar-aware TLV custom mutator.
 *
 * The top-level LLVMFuzzerCustomMutator in the harness handles the prefix /
 * tail split; this file implements tlv_custom_mutate() which operates on the
 * tail region only, using current_tlv_fuzz_config to drive structural
 * mutations matching the active TLV grammar.
 */

#include "tlv_mutator.h"
#include <stdbool.h>
#include <string.h>

extern size_t LLVMFuzzerMutate(uint8_t *Data, size_t Size, size_t MaxSize);

tlv_fuzz_config_t current_tlv_fuzz_config = {0};

static size_t pick_tag_length(const tlv_tag_info_t *t, unsigned int Seed)
{
    size_t range = 1;
    if (t->max_len >= t->min_len) {
        range = (size_t) t->max_len - (size_t) t->min_len + 1u;
    }
    return (size_t) t->min_len + ((size_t) Seed % range);
}

static bool pick_entry(const uint8_t *Data,
                       size_t         Size,
                       unsigned int   Seed,
                       size_t        *off,
                       size_t        *len)
{
    size_t count = 0, cursor = 0;
    while (cursor + 2 <= Size) {
        uint8_t l = Data[cursor + 1];
        if (cursor + 2 + l > Size) {
            break;
        }
        count++;
        cursor += 2 + l;
    }
    if (count == 0) {
        return false;
    }
    size_t target = Seed % count;
    cursor        = 0;
    for (size_t i = 0; i < count; i++) {
        uint8_t l = Data[cursor + 1];
        if (i == target) {
            *off = cursor;
            *len = 2 + l;
            return true;
        }
        cursor += 2 + l;
    }
    return false;
}

static size_t build_complete(uint8_t *Data, size_t MaxSize, unsigned int Seed)
{
    size_t pos = 0;
    for (size_t i = 0; i < current_tlv_fuzz_config.num_tags; i++) {
        const tlv_tag_info_t *t   = &current_tlv_fuzz_config.tags_info[i];
        size_t                len = pick_tag_length(t, Seed);
        if (pos + 2 + len > MaxSize) {
            break;
        }
        Data[pos]     = t->tag;
        Data[pos + 1] = (uint8_t) len;
        for (size_t j = 0; j < len; j++) {
            Data[pos + 2 + j] = (uint8_t) (Seed >> ((j + i) % 24));
        }
        pos += 2 + len;
        Seed = Seed * 1103515245u + 12345u;
    }
    return pos;
}

static size_t append_tag(uint8_t *Data, size_t Size, size_t MaxSize, unsigned int Seed)
{
    const tlv_tag_info_t *t
        = &current_tlv_fuzz_config.tags_info[Seed % current_tlv_fuzz_config.num_tags];
    size_t len = pick_tag_length(t, Seed >> 8);
    if (Size + 2 + len > MaxSize) {
        return Size;
    }
    Data[Size]     = t->tag;
    Data[Size + 1] = (uint8_t) len;
    for (size_t i = 0; i < len; i++) {
        Data[Size + 2 + i] = (uint8_t) (Seed >> (i % 8));
    }
    return Size + 2 + len;
}

static size_t delete_entry(uint8_t *Data, size_t Size, unsigned int Seed)
{
    size_t off, len;
    if (!pick_entry(Data, Size, Seed, &off, &len) || len >= Size) {
        return Size;
    }
    memmove(&Data[off], &Data[off + len], Size - off - len);
    return Size - len;
}

static size_t duplicate_entry(uint8_t *Data, size_t Size, size_t MaxSize, unsigned int Seed)
{
    size_t off, len;
    if (!pick_entry(Data, Size, Seed, &off, &len) || Size + len > MaxSize) {
        return Size;
    }
    memmove(&Data[Size], &Data[off], len);
    return Size + len;
}

static size_t corrupt_length(uint8_t *Data, size_t Size, unsigned int Seed)
{
    size_t off, len;
    if (!pick_entry(Data, Size, Seed, &off, &len)) {
        return Size;
    }
    uint8_t choices[] = {0x00, 0xFF, (uint8_t) (Seed >> 12)};
    Data[off + 1]     = choices[(Seed >> 8) % 3];
    return Size;
}

static size_t truncate_buf(size_t Size, unsigned int Seed)
{
    return (Size > 1) ? 1 + (Seed % (Size - 1)) : Size;
}

size_t tlv_custom_mutate(uint8_t *Data, size_t Size, size_t MaxSize, unsigned int Seed)
{
    if (current_tlv_fuzz_config.num_tags == 0) {
        return LLVMFuzzerMutate(Data, Size, MaxSize);
    }

    unsigned int roll = Seed % 100;

    if (roll < 10) {
        return build_complete(Data, MaxSize, Seed);
    }
    if (roll < 30) {
        return append_tag(Data, Size, MaxSize, Seed);
    }
    if (roll < 40) {
        return duplicate_entry(Data, Size, MaxSize, Seed);
    }
    if (roll < 50) {
        return delete_entry(Data, Size, Seed);
    }
    if (roll < 80) {
        return LLVMFuzzerMutate(Data, Size, MaxSize);
    }
    if (roll < 90) {
        return corrupt_length(Data, Size, Seed);
    }
    return truncate_buf(Size, Seed);
}
