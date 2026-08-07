/*
 * Grammar-aware TLV mutator.
 *
 * Keeps mutated payloads parseable so the fuzzer spends its budget on values
 * rather than on framing. Set current_tlv_fuzz_config (directly, or via
 * fuzz_tlv_dispatch_mutate() for the per-command case) and call
 * tlv_custom_mutate().
 *
 * Encoding: DER, because that is what lib_tlv actually parses. Both the tag and
 * the length go through get_der_value_as_uint32() (lib_tlv/tlv_library.c), so a
 * first byte with 0x80 set is read as a long-form prefix -- n follow-on
 * big-endian bytes -- not as the value itself.
 *
 * Writing a tag >= 0x80 as one raw byte therefore produces a blob the parser
 * rejects at its first byte, before find_handler() is reached. Below 0x80 DER
 * short form is that same single byte.
 */

#include "tlv_mutator.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

extern size_t LLVMFuzzerMutate(uint8_t *Data, size_t Size, size_t MaxSize);

tlv_fuzz_config_t current_tlv_fuzz_config = {0};

/*  ─── DER encoding helpers ─────────────────────────────────────────────── */

/* Write a single TLV "value" (tag or length) using DER short / long form.
 * Returns the number of bytes written, or 0 if out_size is too small.
 *
 * < 0x80 :          [value]
 * >= 0x80:          [0x80 | n][big-endian bytes]
 *
 * tlv_library.c reads tags via get_der_value_as_uint32 (4-byte cap) and
 * lengths via get_der_value_as_uint16 (2-byte cap). The cap fits any value
 * we emit here since tag_info fields are uint8_t.
 */
static size_t der_emit(uint32_t value, uint8_t *out, size_t out_size)
{
    if (value < 0x80) {
        if (out_size < 1) {
            return 0;
        }
        out[0] = (uint8_t) value;
        return 1;
    }

    uint8_t  buf[4];
    uint32_t v = value;
    size_t   n = 0;
    while (v != 0 && n < sizeof(buf)) {
        buf[n++] = (uint8_t) (v & 0xFFu);
        v >>= 8;
    }
    if (n == 0) {
        n      = 1;
        buf[0] = 0;
    }
    if (out_size < (size_t) 1 + n) {
        return 0;
    }
    out[0] = (uint8_t) (0x80u | n);
    for (size_t i = 0; i < n; i++) {
        out[1 + i] = buf[n - 1 - i];
    }
    return 1 + n;
}

/* DER-encoded size of a value (without writing bytes). Mirrors der_emit. */
static size_t der_size(uint32_t value)
{
    if (value < 0x80) {
        return 1;
    }
    size_t n = 0;
    while (value != 0) {
        value >>= 8;
        n++;
    }
    return 1 + (n == 0 ? 1 : n);
}

/* Read one DER-encoded uint at *off; on success advance *off and store
 * the parsed value. Returns false if the encoding is malformed or runs
 * past the buffer. The walker is shared by pick_entry / delete_entry /
 * duplicate_entry / corrupt_length so they all agree with the format
 * emitted by build_complete / append_tag (and with lib_tlv/tlv_library.c). */
static bool der_read(const uint8_t *data, size_t size, size_t *off, uint32_t *value)
{
    if (*off >= size) {
        return false;
    }
    uint8_t first = data[*off];
    if ((first & 0x80u) == 0) {
        *value = first;
        *off += 1;
        return true;
    }
    uint8_t n = first & 0x7Fu;
    if (n == 0 || n > sizeof(uint32_t)) {
        return false;
    }
    if (*off + 1 + n > size) {
        return false;
    }
    uint32_t v = 0;
    for (uint8_t i = 0; i < n; i++) {
        v = (v << 8) | data[*off + 1 + i];
    }
    *value = v;
    *off += 1 + n;
    return true;
}

/* Locate one TLV entry by index Seed mod count. *off receives the byte
 * offset of the entry's first tag byte, *len its full byte length. */
static bool pick_entry(const uint8_t *Data,
                       size_t         Size,
                       unsigned int   Seed,
                       size_t        *off,
                       size_t        *len)
{
    size_t   cursor = 0;
    size_t   count  = 0;
    uint32_t tag_v;
    uint32_t len_v;

    while (cursor < Size) {
        size_t entry_start = cursor;
        if (!der_read(Data, Size, &cursor, &tag_v)) {
            break;
        }
        if (!der_read(Data, Size, &cursor, &len_v)) {
            break;
        }
        if (cursor + len_v > Size) {
            break;
        }
        cursor += len_v;
        (void) entry_start;
        count++;
    }
    if (count == 0) {
        return false;
    }
    size_t target = Seed % count;

    cursor = 0;
    for (size_t i = 0; i < count; i++) {
        size_t entry_start = cursor;
        if (!der_read(Data, Size, &cursor, &tag_v)) {
            return false;
        }
        if (!der_read(Data, Size, &cursor, &len_v)) {
            return false;
        }
        if (cursor + len_v > Size) {
            return false;
        }
        cursor += len_v;
        if (i == target) {
            *off = entry_start;
            *len = cursor - entry_start;
            return true;
        }
    }
    return false;
}

/*  ─── Structural mutations ─────────────────────────────────────────────── */

static size_t pick_tag_length(const tlv_tag_info_t *t, unsigned int Seed)
{
    size_t range = 1;
    if (t->max_len >= t->min_len) {
        range = (size_t) t->max_len - (size_t) t->min_len + 1u;
    }
    return (size_t) t->min_len + ((size_t) Seed % range);
}

/* Emit one DER-encoded TLV entry [tag][len][value] at out[0..]. Returns
 * the byte count, or 0 if max_size is too small. */
static size_t emit_entry(const tlv_tag_info_t *t,
                         uint8_t              *out,
                         size_t                max_size,
                         unsigned int          seed,
                         size_t                value_len)
{
    size_t tag_bytes = der_emit(t->tag, out, max_size);
    if (tag_bytes == 0) {
        return 0;
    }
    size_t len_bytes = der_emit((uint32_t) value_len, out + tag_bytes, max_size - tag_bytes);
    if (len_bytes == 0) {
        return 0;
    }
    size_t header = tag_bytes + len_bytes;
    if (header + value_len > max_size) {
        return 0;
    }
    for (size_t i = 0; i < value_len; i++) {
        out[header + i] = (uint8_t) (seed >> ((i + (size_t) t->tag) % 24));
    }
    return header + value_len;
}

static size_t build_complete(uint8_t *Data, size_t MaxSize, unsigned int Seed)
{
    size_t pos = 0;
    for (size_t i = 0; i < current_tlv_fuzz_config.num_tags; i++) {
        const tlv_tag_info_t *t    = &current_tlv_fuzz_config.tags_info[i];
        size_t                len  = pick_tag_length(t, Seed);
        size_t                need = der_size(t->tag) + der_size((uint32_t) len) + len;
        if (pos + need > MaxSize) {
            break;
        }
        size_t wrote = emit_entry(t, Data + pos, MaxSize - pos, Seed, len);
        if (wrote == 0) {
            break;
        }
        pos += wrote;
        Seed = Seed * 1103515245u + 12345u;
    }
    return pos;
}

static size_t append_tag(uint8_t *Data, size_t Size, size_t MaxSize, unsigned int Seed)
{
    if (current_tlv_fuzz_config.num_tags == 0) {
        return Size;
    }
    const tlv_tag_info_t *t
        = &current_tlv_fuzz_config.tags_info[Seed % current_tlv_fuzz_config.num_tags];
    size_t len  = pick_tag_length(t, Seed >> 8);
    size_t need = der_size(t->tag) + der_size((uint32_t) len) + len;
    if (Size + need > MaxSize) {
        return Size;
    }
    size_t wrote = emit_entry(t, Data + Size, MaxSize - Size, Seed, len);
    if (wrote == 0) {
        return Size;
    }
    return Size + wrote;
}

static size_t delete_entry(uint8_t *Data, size_t Size, unsigned int Seed)
{
    size_t off;
    size_t len;
    if (!pick_entry(Data, Size, Seed, &off, &len) || len >= Size) {
        return Size;
    }
    memmove(&Data[off], &Data[off + len], Size - off - len);
    return Size - len;
}

static size_t duplicate_entry(uint8_t *Data, size_t Size, size_t MaxSize, unsigned int Seed)
{
    size_t off;
    size_t len;
    if (!pick_entry(Data, Size, Seed, &off, &len) || Size + len > MaxSize) {
        return Size;
    }
    memmove(&Data[Size], &Data[off], len);
    return Size + len;
}

/* Corrupt the chosen entry's length byte while preserving DER framing
 * shape. Most useful when the value is in long form: we either zero the
 * length, force a wildly oversized claim, or splice a random byte. */
static size_t corrupt_length(uint8_t *Data, size_t Size, unsigned int Seed)
{
    size_t off;
    size_t len;
    if (!pick_entry(Data, Size, Seed, &off, &len)) {
        return Size;
    }
    size_t   tag_off = off;
    uint32_t tag_v;
    if (!der_read(Data, Size, &tag_off, &tag_v)) {
        return Size;
    }
    if (tag_off >= Size) {
        return Size;
    }
    uint8_t choices[] = {0x00, 0xFF, (uint8_t) (Seed >> 12)};
    Data[tag_off]     = choices[(Seed >> 8) % 3];
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
