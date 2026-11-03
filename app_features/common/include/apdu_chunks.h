/*****************************************************************************
 *   (c) 2026 Ledger SAS.
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

/**
 * @file apdu_chunks.h
 * @brief Chunked APDU payload reassembly shared by the SDK app features.
 *
 * The first chunk (P2=APDU_CHUNKS_P2_FIRST) is prefixed with a 2-byte big-endian
 * total payload length; continuation chunks are appended directly.
 * A single reassembly buffer is shared by all the features: a payload in progress
 * is owned by the feature that started it.
 */

#pragma once

/* Includes ------------------------------------------------------------------*/
#include <stddef.h>
#include <stdint.h>

/* Exported enumerations -----------------------------------------------------*/
typedef enum {
    APDU_CHUNKS_PENDING,   ///< More chunks needed
    APDU_CHUNKS_COMPLETE,  ///< Full payload assembled; out_buf/out_len are valid
    APDU_CHUNKS_ERROR,     ///< Protocol error; the payload in progress is dropped
} apdu_chunks_status_t;

/** Features sharing the reassembly buffer. */
typedef enum {
    APDU_CHUNKS_OWNER_ADDRESS_BOOK = 1,
    APDU_CHUNKS_OWNER_TRANSACTION_CHECK,
} apdu_chunks_owner_t;

/* Exported defines   --------------------------------------------------------*/
/** P2 value for the first (or only) APDU chunk. */
#define APDU_CHUNKS_P2_FIRST 0x00
/** P2 value for continuation APDU chunks. */
#define APDU_CHUNKS_P2_NEXT  0x80

/* Exported functions prototypes--------------------------------------------- */

/**
 * @brief Reassemble a multi-chunk APDU payload.
 *
 * @param[in]  owner    Feature owning the payload in progress
 * @param[in]  data     Chunk data (including the 2-byte header on the first chunk)
 * @param[in]  len      Length of this chunk
 * @param[in]  p2       APDU_CHUNKS_P2_FIRST or APDU_CHUNKS_P2_NEXT
 * @param[out] out_buf  Set to the assembled buffer when APDU_CHUNKS_COMPLETE
 * @param[out] out_len  Set to the assembled length when APDU_CHUNKS_COMPLETE
 * @return apdu_chunks_status_t
 */
apdu_chunks_status_t apdu_chunks_reassemble(apdu_chunks_owner_t owner,
                                            const uint8_t      *data,
                                            size_t              len,
                                            uint8_t             p2,
                                            uint8_t           **out_buf,
                                            size_t             *out_len);
