/*****************************************************************************
 *   (c) 2020 Ledger SAS.
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

#include <stdio.h>    // snprintf
#include <string.h>   // memset, strlen
#include <stddef.h>   // size_t
#include <stdint.h>   // uint*_t
#include <stdbool.h>  // bool

#include "os_utils.h"
#include "bip32.h"
#include "read.h"

/**
 * @brief Read a BIP32 path from a byte buffer.
 *
 * @param[in] in The input byte buffer.
 * @param[in] in_len The length of the input byte buffer.
 * @param[out] out The output buffer to store the BIP32 path.
 * @param[in] out_len The length of the output buffer.
 * @return true if the BIP32 path was successfully read, false otherwise.
 */
bool bip32_path_read(const uint8_t *in, size_t in_len, uint32_t *out, size_t out_len)
{
    if (out_len == 0 || out_len > MAX_BIP32_PATH) {
        return false;
    }

    size_t offset = 0;

    for (size_t i = 0; i < out_len; i++) {
        if (offset + 4 > in_len) {
            return false;
        }
        out[i] = read_u32_be(in, offset);
        offset += 4;
    }

    return true;
}

/**
 * @brief Parse a BIP32 path from a byte buffer.
 *
 * @param[in] dataBuffer The input byte buffer.
 * @param[in,out] dataLength The length of the input byte buffer. Updated after parsing.
 * @param[out] bip32 The output BIP32 path structure.
 * @return Pointer to the advanced buffer position on success, NULL on error.
 */
const uint8_t *bip32_path_parse(const uint8_t *dataBuffer, uint8_t *dataLength, path_bip32_t *bip32)
{
    if ((dataBuffer == NULL) || (dataLength == NULL) || (bip32 == NULL) || (*dataLength < 1)) {
        return NULL;
    }

    bip32->length = *dataBuffer;

    dataBuffer++;
    (*dataLength)--;

    if (*dataLength < sizeof(uint32_t) * (bip32->length)) {
        return NULL;
    }

    if (bip32_path_read(dataBuffer, (size_t) *dataLength, bip32->path, (size_t) bip32->length)
        == false) {
        return NULL;
    }
    dataBuffer += bip32->length * sizeof(uint32_t);
    *dataLength -= bip32->length * sizeof(uint32_t);

    return dataBuffer;
}

/**
 * @brief Format a BIP32 path as a string.
 *
 * @param[in] bip32_path The BIP32 path array.
 * @param[in] bip32_path_len The length of the BIP32 path array.
 * @param[out] out The output buffer to store the formatted string.
 * @param[in] out_len The length of the output buffer.
 * @return true if the BIP32 path was successfully formatted, false otherwise.
 */
bool bip32_path_format(const uint32_t *bip32_path, size_t bip32_path_len, char *out, size_t out_len)
{
    if (bip32_path_len == 0 || bip32_path_len > MAX_BIP32_PATH) {
        return false;
    }

    size_t offset = 0;

    for (uint16_t i = 0; i < bip32_path_len; i++) {
        size_t written;

        snprintf(out + offset, out_len - offset, "%u", bip32_path[i] & 0x7FFFFFFFu);
        written = strlen(out + offset);
        if (written == 0 || written >= out_len - offset) {
            memset(out, 0, out_len);
            return false;
        }
        offset += written;

        if ((bip32_path[i] & 0x80000000u) != 0) {
            snprintf(out + offset, out_len - offset, "'");
            written = strlen(out + offset);
            if (written == 0 || written >= out_len - offset) {
                memset(out, 0, out_len);
                return false;
            }
            offset += written;
        }

        if (i != bip32_path_len - 1) {
            snprintf(out + offset, out_len - offset, "/");
            written = strlen(out + offset);
            if (written == 0 || written >= out_len - offset) {
                memset(out, 0, out_len);
                return false;
            }
            offset += written;
        }
    }

    return true;
}

/**
 * @brief Format a BIP32 path as a string.
 *
 * @param[in] bip32_path The BIP32 path array.
 * @param[in] bip32_path_len The length of the BIP32 path array.
 * @param[out] out The output buffer to store the formatted string.
 * @param[in] out_len The length of the output buffer.
 * @return true if the BIP32 path was successfully formatted, false otherwise.
 */
bool bip32_path_format_simple(path_bip32_t *bip32, char *out, size_t out_len)
{
    if (bip32 == NULL || out == NULL || out_len == 0) {
        return false;
    }
    return bip32_path_format(bip32->path, bip32->length, out, out_len);
}

/**
 * @brief Encode a BIP32 path into a byte buffer.
 *
 * @param[in] bip32 The input BIP32 path structure.
 * @param[out] out The output byte buffer to store the encoded BIP32 path.
 * @return The number of bytes written to the output buffer, or 0 on error.
 */
size_t bip32_path_encode(path_bip32_t *bip32, uint8_t *out)
{
    size_t offset = 0;
    if (bip32 == NULL || bip32->length == 0 || bip32->length > MAX_BIP32_PATH || out == NULL) {
        return 0;
    }

    out[offset++] = bip32->length;
    for (uint8_t i = 0; i < bip32->length; i++) {
        U4BE_ENCODE(out, offset, bip32->path[i]);
        offset += 4;
    }
    return offset;
}
