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

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "os_helpers.h"
#include "os_print.h"
#include "os_utils.h"
#include "apdu_chunks.h"
#if defined(HAVE_ADDRESS_BOOK)
#include "address_book.h"
#endif
#if defined(HAVE_TX_CHECK)
#include "transaction_check.h"
#endif

#if defined(HAVE_ADDRESS_BOOK) || defined(HAVE_TX_CHECK)

/* Private defines------------------------------------------------------------*/
#if defined(HAVE_ADDRESS_BOOK)
#define AB_CHUNKS_SIZE ADDRESS_BOOK_MAX_CHUNKED_PAYLOAD
#else
#define AB_CHUNKS_SIZE 0
#endif
#if defined(HAVE_TX_CHECK)
#define TX_CHECK_CHUNKS_SIZE TRANSACTION_CHECK_MAX_PAYLOAD_SIZE
#else
#define TX_CHECK_CHUNKS_SIZE 0
#endif

/** Largest payload among the enabled features. */
#define APDU_CHUNKS_BUFFER_SIZE \
    ((AB_CHUNKS_SIZE > TX_CHECK_CHUNKS_SIZE) ? AB_CHUNKS_SIZE : TX_CHECK_CHUNKS_SIZE)

/* Private variables ---------------------------------------------------------*/
static uint8_t             s_chunk_buf[APDU_CHUNKS_BUFFER_SIZE] = {0};
static uint16_t            s_chunk_total                        = 0;
static uint16_t            s_chunk_received                     = 0;
static apdu_chunks_owner_t s_chunk_owner;

/* Exported functions --------------------------------------------------------*/
apdu_chunks_status_t apdu_chunks_reassemble(apdu_chunks_owner_t owner,
                                            const uint8_t      *data,
                                            size_t              len,
                                            uint8_t             p2,
                                            uint8_t           **out_buf,
                                            size_t             *out_len)
{
    if ((p2 != APDU_CHUNKS_P2_FIRST) && (p2 != APDU_CHUNKS_P2_NEXT)) {
        PRINTF("[APDU chunks] Invalid P2: 0x%02x\n", p2);
        s_chunk_total = 0;
        return APDU_CHUNKS_ERROR;
    }
    if (p2 == APDU_CHUNKS_P2_FIRST) {
        // A first chunk drops the payload in progress, even when it is invalid
        s_chunk_total = 0;
        if (len < 2) {
            PRINTF("[APDU chunks] First chunk too short (no length header)\n");
            return APDU_CHUNKS_ERROR;
        }
        s_chunk_total    = U2BE(data, 0);
        s_chunk_received = 0;
        s_chunk_owner    = owner;
        data += 2;
        len -= 2;
        if (s_chunk_total == 0 || s_chunk_total > sizeof(s_chunk_buf)) {
            PRINTF("[APDU chunks] Invalid total length: %u\n", s_chunk_total);
            s_chunk_total = 0;
            return APDU_CHUNKS_ERROR;
        }
    }
    else {
        if (s_chunk_total == 0) {
            PRINTF("[APDU chunks] Unexpected continuation chunk\n");
            return APDU_CHUNKS_ERROR;
        }
        if (owner != s_chunk_owner) {
            PRINTF("[APDU chunks] Continuation chunk from feature %d, payload owned by %d\n",
                   owner,
                   s_chunk_owner);
            s_chunk_total = 0;
            return APDU_CHUNKS_ERROR;
        }
    }

    if ((s_chunk_received > s_chunk_total) || (len > (s_chunk_total - s_chunk_received))) {
        PRINTF(
            "[APDU chunks] Chunk overflow: %u + %zu > %u\n", s_chunk_received, len, s_chunk_total);
        s_chunk_total = 0;
        return APDU_CHUNKS_ERROR;
    }
    memmove(s_chunk_buf + s_chunk_received, data, len);
    s_chunk_received += (uint16_t) len;

    if (s_chunk_received < s_chunk_total) {
        return APDU_CHUNKS_PENDING;
    }

    *out_buf      = s_chunk_buf;
    *out_len      = s_chunk_received;
    s_chunk_total = 0;
    return APDU_CHUNKS_COMPLETE;
}
#endif  // HAVE_ADDRESS_BOOK || HAVE_TX_CHECK
