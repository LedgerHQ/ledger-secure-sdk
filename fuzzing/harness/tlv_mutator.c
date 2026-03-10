#include "tlv_mutator.h"
#include <string.h>
#include <stdbool.h>

extern size_t LLVMFuzzerMutate(uint8_t *Data, size_t Size, size_t MaxSize);

tlv_fuzz_config_t current_tlv_fuzz_config = {0};
static bool       is_recursing            = false;

// Pick a random TLV entry in the buffer. Returns false if none found.
static bool pick_entry(const uint8_t *Data,
                       size_t         Size,
                       unsigned int   Seed,
                       size_t        *off,
                       size_t        *len)
{
    // Count well-formed entries
    size_t count  = 0;
    size_t cursor = 0;
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

// Build a complete TLV message containing all grammar tags with random values.
static size_t build_complete(uint8_t *Data, size_t MaxSize, unsigned int Seed)
{
    size_t pos = 0;
    for (size_t i = 0; i < current_tlv_fuzz_config.num_tags; i++) {
        const tlv_tag_info_t *t   = &current_tlv_fuzz_config.tags_info[i];
        uint8_t               rng = (t->max_len >= t->min_len) ? (t->max_len - t->min_len + 1) : 1;
        uint8_t               len = t->min_len + (Seed % rng);

        if (pos + 2 + len > MaxSize) {
            break;
        }
        Data[pos]     = t->tag;
        Data[pos + 1] = len;
        for (uint8_t j = 0; j < len; j++) {
            Data[pos + 2 + j] = (uint8_t) (Seed >> ((j + i) % 24));
        }
        pos += 2 + len;
        Seed = Seed * 1103515245u + 12345u;
    }
    return pos;
}

// Append one grammar-valid TLV entry at the end.
static size_t append_tag(uint8_t *Data, size_t Size, size_t MaxSize, unsigned int Seed)
{
    const tlv_tag_info_t *t
        = &current_tlv_fuzz_config.tags_info[Seed % current_tlv_fuzz_config.num_tags];
    uint8_t rng = (t->max_len >= t->min_len) ? (t->max_len - t->min_len + 1) : 1;
    uint8_t len = t->min_len + ((Seed >> 8) % rng);

    if (Size + 2 + len > MaxSize) {
        return Size;
    }
    Data[Size]     = t->tag;
    Data[Size + 1] = len;
    for (uint8_t i = 0; i < len; i++) {
        Data[Size + 2 + i] = (uint8_t) (Seed >> (i % 8));
    }
    return Size + 2 + len;
}

// Remove a random TLV entry from the buffer.
static size_t delete_entry(uint8_t *Data, size_t Size, unsigned int Seed)
{
    size_t off, len;
    if (!pick_entry(Data, Size, Seed, &off, &len) || len >= Size) {
        return Size;
    }
    memmove(&Data[off], &Data[off + len], Size - off - len);
    return Size - len;
}

// Duplicate a random TLV entry at the end.
static size_t duplicate_entry(uint8_t *Data, size_t Size, size_t MaxSize, unsigned int Seed)
{
    size_t off, len;
    if (!pick_entry(Data, Size, Seed, &off, &len) || Size + len > MaxSize) {
        return Size;
    }
    memcpy(&Data[Size], &Data[off], len);
    return Size + len;
}

// Corrupt the length byte of a random entry.
static size_t corrupt_length(uint8_t *Data, size_t Size, unsigned int Seed)
{
    size_t off, len;
    if (!pick_entry(Data, Size, Seed, &off, &len)) {
        return Size;
    }
    // Pick between: 0, 0xFF, random byte
    uint8_t choices[] = {0x00, 0xFF, (uint8_t) (Seed >> 12)};
    Data[off + 1]     = choices[(Seed >> 8) % 3];
    return Size;
}

// Truncate the buffer at a random offset.
static size_t truncate_buf(size_t Size, unsigned int Seed)
{
    return (Size > 1) ? 1 + (Seed % (Size - 1)) : Size;
}

size_t LLVMFuzzerCustomMutator(uint8_t *Data, size_t Size, size_t MaxSize, unsigned int Seed)
{
    // Guard against recursion from LLVMFuzzerMutate
    if (is_recursing) {
        if (Size > 0) {
            Data[Seed % Size] ^= (Seed >> 8) & 0xFF;
        }
        return Size;
    }

    // Fallback to default mutator if no grammar is set
    if (current_tlv_fuzz_config.num_tags == 0) {
        is_recursing = true;
        Size         = LLVMFuzzerMutate(Data, Size, MaxSize);
        is_recursing = false;
        return Size;
    }

    unsigned int roll = Seed % 100;

    // 50% structure-preserving
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

    // 50% error-inducing (let the default mutator hit parser error paths)
    if (roll < 80) {
        is_recursing = true;
        Size         = LLVMFuzzerMutate(Data, Size, MaxSize);
        is_recursing = false;
        return Size;
    }
    if (roll < 90) {
        return corrupt_length(Data, Size, Seed);
    }
    return truncate_buf(Size, Seed);
}
