/*****************************************************************************
 *   (c) 2025 Ledger SAS.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *****************************************************************************/

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "os_utils.h"
#include "os_pic.h"
#include "cx.h"

#include "tlv_library.h"

#define DER_LONG_FORM_FLAG        0x80  // 8th bit set
#define DER_FIRST_BYTE_VALUE_MASK 0x7f

/**
 * @brief Enforce that a TLV data contains the expected uint8 value.
 *
 * @param[in] data TLV data to check
 * @param[in] expected_value Expected uint8 value
 * @return true if the value matches, false otherwise
 */
bool tlv_enforce_u8_value(const tlv_data_t *data, uint8_t expected_value)
{
    uint8_t value = 0;
    if (!get_uint8_t_from_tlv_data(data, &value)) {
        PRINTF("Failed to extract uint8 value from TLV data (tag 0x%x)\n", data->tag);
        return false;
    }
    if (value != expected_value) {
        PRINTF(
            "Value mismatch for tag 0x%x: expected %u, got %u\n", data->tag, expected_value, value);
        return false;
    }
    return true;
}

/**
 * @brief Extract a uint64_t value from TLV data.
 *
 * The value is interpreted as a big-endian unsigned integer.
 * The TLV length must be between 1 and 8 bytes; shorter payloads are zero-padded.
 * See also https://en.wikipedia.org/wiki/X.690 for DER encoding rules.
 *
 * @param[in]  data  TLV data to read from
 * @param[out] value Parsed uint64_t value
 * @return true on success, false if @p data is NULL or the length is out of range
 */
bool get_uint64_t_from_tlv_data(const tlv_data_t *data, uint64_t *value)
{
    if (data == NULL || value == NULL) {
        PRINTF("Received NULL parameter\n");
        return false;
    }
    if (data->value.size == 0 || data->value.size > sizeof(uint64_t)) {
        PRINTF("Invalid length (%u bytes) for tag 0x%x (expected 1-%zu bytes)\n",
               data->value.size,
               data->tag,
               sizeof(uint64_t));
        return false;
    }

    // Pad with zeros before calling U8BE()
    uint8_t buffer[sizeof(uint64_t)] = {0};
    memcpy(buffer + (sizeof(buffer) - data->value.size), data->value.ptr, data->value.size);

    *value = U8BE(buffer, 0);
    return true;
}

/**
 * @brief Extract a uint32_t value from TLV data.
 *
 * Delegates to get_uint64_t_from_tlv_data() and rejects values exceeding `UINT32_MAX`.
 *
 * @param[in]  data  TLV data to read from
 * @param[out] value Parsed uint32_t value
 * @return true on success, false if @p data is NULL, the length is invalid,
 *         or the value overflows uint32_t
 */
bool get_uint32_t_from_tlv_data(const tlv_data_t *data, uint32_t *value)
{
    if (data == NULL || value == NULL) {
        PRINTF("Received NULL parameter\n");
        return false;
    }
    uint64_t tmp_value;
    if (!get_uint64_t_from_tlv_data(data, &tmp_value) || tmp_value > UINT32_MAX) {
        return false;
    }
    *value = (uint32_t) tmp_value;
    return true;
}

/**
 * @brief Extract a uint16_t value from TLV data.
 *
 * Delegates to get_uint64_t_from_tlv_data() and rejects values exceeding `UINT16_MAX`.
 *
 * @param[in]  data  TLV data to read from
 * @param[out] value Parsed uint16_t value
 * @return true on success, false if @p data is NULL, the length is invalid,
 *         or the value overflows uint16_t
 */
bool get_uint16_t_from_tlv_data(const tlv_data_t *data, uint16_t *value)
{
    if (data == NULL || value == NULL) {
        PRINTF("Received NULL parameter\n");
        return false;
    }
    uint64_t tmp_value;
    if (!get_uint64_t_from_tlv_data(data, &tmp_value) || (tmp_value > UINT16_MAX)) {
        return false;
    }
    *value = (uint16_t) tmp_value;
    return true;
}

/**
 * @brief Extract a uint8_t value from TLV data.
 *
 * Delegates to get_uint64_t_from_tlv_data() and rejects values exceeding `UINT8_MAX`.
 *
 * @param[in]  data  TLV data to read from
 * @param[out] value Parsed uint8_t value
 * @return true on success, false if @p data is NULL, the length is invalid,
 *         or the value overflows uint8_t
 */
bool get_uint8_t_from_tlv_data(const tlv_data_t *data, uint8_t *value)
{
    if (data == NULL || value == NULL) {
        PRINTF("Received NULL parameter\n");
        return false;
    }
    uint64_t tmp_value;
    if (!get_uint64_t_from_tlv_data(data, &tmp_value) || (tmp_value > UINT8_MAX)) {
        return false;
    }
    *value = (uint8_t) tmp_value;
    return true;
}

/**
 * @brief Extract a boolean value from TLV data.
 *
 * The TLV value must be exactly 1 byte with value 0 or 1.
 * Any other value causes the function to return false.
 *
 * @param[in]  data  TLV data to read from
 * @param[out] value Parsed boolean value
 * @return true on success, false if @p data is NULL, the length is invalid,
 *         or the byte is neither 0 nor 1
 */
bool get_bool_from_tlv_data(const tlv_data_t *data, bool *value)
{
    if (data == NULL || value == NULL) {
        PRINTF("Received NULL parameter\n");
        return false;
    }
    uint8_t tmp_value;
    if (!get_uint8_t_from_tlv_data(data, &tmp_value) || (tmp_value > 1)) {
        return false;
    }
    *value = (tmp_value == 1);
    return true;
}

/**
 * @brief Extract a sized buffer from TLV data (zero-copy).
 *
 * Populates @p out with a pointer into the TLV payload and its length.
 * No data is copied; the caller must not outlive the original payload.
 *
 * @param[in]  data     TLV data to read from
 * @param[out] out      `buffer_t` pointing into the TLV value bytes
 * @param[in]  min_size Minimum acceptable length (0 to skip the lower-bound check)
 * @param[in]  max_size Maximum acceptable length (0 to skip the upper-bound check)
 * @return true on success, false if @p data is NULL or the length is outside the allowed range
 */
bool get_buffer_from_tlv_data(const tlv_data_t *data,
                              buffer_t         *out,
                              uint16_t          min_size,
                              uint16_t          max_size)
{
    if (data == NULL || out == NULL) {
        PRINTF("Received NULL parameter\n");
        return false;
    }

    if (min_size != 0 && data->value.size < min_size) {
        PRINTF("Expected at least %d bytes, found %d\n", min_size, data->value.size);
        return false;
    }
    if (max_size != 0 && data->value.size > max_size) {
        PRINTF("Expected at most %d bytes, found %d\n", max_size, data->value.size);
        return false;
    }
    out->size = data->value.size;
    out->ptr  = data->value.ptr;
    return true;
}

/**
 * @brief Copy a NUL-terminated string from TLV data.
 *
 * Copies the TLV bytes into @p out and appends a NUL terminator.
 * The function rejects payloads that contain embedded NUL bytes.
 *
 * @param[in]  data       TLV data to read from
 * @param[out] out        Destination buffer (must hold at least `value.size + 1` bytes)
 * @param[in]  min_length Minimum acceptable string length in bytes, excluding NUL
 *                        (0 to skip the lower-bound check)
 * @param[in]  out_size   Size of @p out in bytes including the NUL terminator
 *                        (0 to skip the upper-bound check)
 * @return true on success, false if @p data is NULL, an embedded NUL is found,
 *         or the length is outside the allowed range
 */
bool get_string_from_tlv_data(const tlv_data_t *data,
                              char             *out,
                              uint16_t          min_length,
                              uint16_t          out_size)
{
    if (data == NULL || out == NULL) {
        PRINTF("Received NULL parameter\n");
        return false;
    }

    // Reject TLV strings with embedded null bytes
    size_t actual_length = 0;
    if (data->value.size > 0) {
        actual_length = strnlen((const char *) data->value.ptr, data->value.size);
    }
    if (actual_length != data->value.size) {
        PRINTF("Embedded null byte at offset %u\n", (unsigned) actual_length);
        return false;
    }

    if (min_length != 0 && data->value.size < min_length) {
        PRINTF("Expected at least %u bytes, found %u\n", min_length, data->value.size);
        return false;
    }
    // The input is not '\0' terminated
    if (out_size != 0 && data->value.size + 1 > out_size) {
        PRINTF("Expected at most %u bytes, found %u (+1)\n", out_size, data->value.size);
        return false;
    }

    if (data->value.size > 0) {
        memcpy(out, data->value.ptr, data->value.size);
    }
    out[data->value.size] = '\0';

    return true;
}

/** Parse DER-encoded value
 *
 * Parses a DER-encoded value (up to 4 bytes long)
 * https://en.wikipedia.org/wiki/X.690
 *
 * @param[in] payload the TLV payload
 * @param[in,out] offset the payload offset
 * @param[out] value the parsed value
 * @return whether it was successful
 */
static bool get_der_value_as_uint32(const buffer_t *payload, size_t *offset, uint32_t *value)
{
    uint8_t byte_length;
    uint8_t buf[sizeof(*value)];

    if (payload == NULL || offset == NULL || value == NULL) {
        PRINTF("Received NULL parameter\n");
        return false;
    }

    if (payload->ptr[*offset] & DER_LONG_FORM_FLAG) {  // long form
        byte_length = payload->ptr[*offset] & DER_FIRST_BYTE_VALUE_MASK;
        *offset += 1;
        if ((*offset + byte_length) > payload->size) {
            PRINTF("TLV payload too small for DER encoded value\n");
            return false;
        }
        if (byte_length > sizeof(buf) || byte_length == 0) {
            PRINTF("Unexpectedly long DER-encoded value (%u bytes)\n", byte_length);
            return false;
        }
        memset(buf, 0, (sizeof(buf) - byte_length));
        memcpy(buf + (sizeof(buf) - byte_length), &payload->ptr[*offset], byte_length);
        *value = U4BE(buf, 0);
        *offset += byte_length;
        return true;
    }
    else {  // short form
        *value = payload->ptr[*offset];
        *offset += 1;
        return true;
    }
}

/**
 * @brief Parse a DER-encoded value and fit it into a uint16_t, or fail.
 *
 * Delegates to get_der_value_as_uint32() and rejects values exceeding `UINT16_MAX`.
 *
 * @param[in]     payload DER-encoded buffer
 * @param[in,out] offset  Current read position, advanced past the consumed bytes
 * @param[out]    value   Parsed uint16_t value
 * @return true on success, false if the underlying parse fails or the value overflows uint16_t
 */
static bool get_der_value_as_uint16(const buffer_t *payload, size_t *offset, uint16_t *value)
{
    uint32_t tmp_value;
    if (!get_der_value_as_uint32(payload, offset, &tmp_value) || (tmp_value > UINT16_MAX)) {
        return false;
    }

    *value = (uint16_t) tmp_value;
    return true;
}

/**
 * @brief Check that every tag in @p tags was received during parsing.
 *
 * Prefer the variadic macro @ref TLV_CHECK_RECEIVED_TAGS which wraps this function.
 *
 * @warning Do not mix tags belonging to different TLV use cases; the flag mapping is
 *          use-case-specific and results would be undefined.
 *
 * @param[in] received   Tag-reception tracker filled in by the parser
 * @param[in] tags       Array of tag values that must have been received
 * @param[in] tag_count  Number of entries in @p tags
 * @return true if every listed tag was received, false if any is missing
 */
bool tlv_check_received_tags(TLV_reception_t received, const TLV_tag_t *tags, size_t tag_count)
{
    for (size_t i = 0; i < tag_count; i++) {
        TLV_flag_t flag = received.tag_to_flag_function(tags[i]);
        if (flag == 0) {
            PRINTF("No flag found for tag 0x%x\n", tags[i]);
            return false;
        }
        if ((received.flags & flag) != flag) {
            PRINTF("Tag 0x%x no received\n", tags[i]);
            return false;
        }
    }
    return true;
}

/**
 * @brief Find the handler registered for a given tag.
 *
 * @param[in] handlers        Array of tag handlers
 * @param[in] handlers_count  Number of entries in @p handlers
 * @param[in] tag             Tag value to look up
 * @return Pointer to the matching handler, or NULL if no handler is registered for @p tag
 */
static const _internal_tlv_handler_t *find_handler(const _internal_tlv_handler_t *handlers,
                                                   uint8_t                        handlers_count,
                                                   TLV_tag_t                      tag)
{
    // check if a handler exists for this tag
    for (uint8_t idx = 0; idx < handlers_count; ++idx) {
        if (handlers[idx].tag == tag) {
            return &handlers[idx];
        }
    }
    return NULL;
}

typedef enum tlv_step_e {
    TLV_TAG,
    TLV_LENGTH,
    TLV_VALUE,
} tlv_step_t;

/**
 * @brief Parse a raw TLV payload using a pre-built handler table.
 *
 * This is the internal engine behind every parser generated by `DEFINE_TLV_PARSER`.
 * Prefer using the generated wrapper for your use case rather than calling this directly.
 *
 * @param[in]  handlers             Array of per-tag handlers
 * @param[in]  handlers_count       Number of entries in @p handlers
 * @param[in]  common_handler       Optional handler called for every tag before the specific one
 *                                  (may be NULL)
 * @param[in]  tag_to_flag_function Maps a tag value to its reception flag bit
 * @param[in]  payload              Raw TLV bytes to parse
 * @param[out] tlv_out              Output struct written into by the handlers
 * @param[out] received_tags_flags  Reception tracker updated as tags are processed
 * @return true on success, false on any parse error or handler failure
 */
bool _parse_tlv_internal(const _internal_tlv_handler_t *handlers,
                         uint8_t                        handlers_count,
                         tlv_handler_cb_t              *common_handler,
                         tag_to_flag_function_t        *tag_to_flag_function,
                         const buffer_t                *payload,
                         void                          *tlv_out,
                         TLV_reception_t               *received_tags_flags)
{
    tlv_step_t                     step = TLV_TAG;
    tlv_data_t                     data;
    size_t                         offset = 0;
    size_t                         tag_start_offset;
    const _internal_tlv_handler_t *handler;
    tlv_handler_cb_t              *fptr;
    uint16_t                       size;
    TLV_flag_t                     flag;

    explicit_bzero(received_tags_flags, sizeof(*received_tags_flags));
    received_tags_flags->tag_to_flag_function = PIC(tag_to_flag_function);

    PRINTF("Parsing TLV payload %.*H\n", payload->size, payload->ptr);

    // handle TLV payload
    while (offset < payload->size || (step == TLV_VALUE && data.value.size == 0)) {
        switch (step) {
            case TLV_TAG:
                tag_start_offset = offset;
                if (!get_der_value_as_uint32(payload, &offset, &data.tag)) {
                    return false;
                }
                handler = find_handler(handlers, handlers_count, data.tag);
                if (handler == NULL) {
                    PRINTF("No handler found for tag 0x%x\n", data.tag);
                    return false;
                }
                step = TLV_LENGTH;
                break;

            case TLV_LENGTH:
                if (!get_der_value_as_uint16(payload, &offset, &size)) {
                    return false;
                }
                data.value.size = size;
                step            = TLV_VALUE;
                break;

            case TLV_VALUE:
                if ((offset + data.value.size) > payload->size) {
                    PRINTF("Error: can't read %d + %d, only %d\n",
                           offset,
                           data.value.size,
                           payload->size);
                    return false;
                }
                if (data.value.size > 0) {
                    data.value.ptr = &payload->ptr[offset];
                    PRINTF("Handling tag 0x%02x length %d value '%.*H'\n",
                           data.tag,
                           data.value.size,
                           data.value.size,
                           data.value.ptr);
                }
                else {
                    data.value.ptr = NULL;
                    PRINTF("Handling tag 0x%02x length %d\n", data.tag, data.value.size);
                }
                offset += data.value.size;

                // Calculate raw TLV start/end to give to the handler
                data.raw.ptr  = &payload->ptr[tag_start_offset];
                data.raw.size = offset - tag_start_offset;

                // Check for duplicate tag
                flag = received_tags_flags->tag_to_flag_function(data.tag);
                if (handler->is_unique && ((received_tags_flags->flags & flag) == flag)) {
                    PRINTF("Tag = %d was already received and is flagged unique\n", data.tag);
                    return false;
                }

                // Call the common handler if there is one
                fptr = PIC(common_handler);
                if (fptr != NULL && !(*fptr)(&data, tlv_out)) {
                    PRINTF("Common handler error while handling tag 0x%x\n", handler->tag);
                    return false;
                }

                // Call this tag's handler if there is one
                fptr = PIC(handler->func);
                if (fptr != NULL && !(*fptr)(&data, tlv_out)) {
                    PRINTF("Handler error while handling tag 0x%x\n", handler->tag);
                    return false;
                }

                // Flag reception after handler callback in case the handler wants to check it
                received_tags_flags->flags |= flag;
                step = TLV_TAG;
                break;

            default:
                return false;
        }
    }
    if (step != TLV_TAG) {
        PRINTF("Error: unexpected end step %d\n", step);
        return false;
    }
    if (offset != payload->size) {
        PRINTF("Error: unexpected data at the end of the TLV payload!\n");
        return false;
    }

    return true;
}
