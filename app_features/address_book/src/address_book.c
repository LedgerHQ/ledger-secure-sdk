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
 * @file address_book.c
 * @brief Address Book APDU dispatcher
 *
 * Entry point for all Address Book sub-commands. Routes each APDU to the
 * appropriate handler based on P1:
 *
 *  - P1 0x01  → Register Identity       (always active)
 *  - P1 0x02  → Edit Contact Name       (always active, single APDU)
 *  - P1 0x03  → Edit Identifier         (always active, multi-chunk)
 *  - P1 0x04  → Edit Scope              (always active, multi-chunk)
 *  - P1 0x11  → Register Ledger Account          (HAVE_ADDRESS_BOOK_LEDGER_ACCOUNT)
 *  - P1 0x12  → Edit Ledger Account              (HAVE_ADDRESS_BOOK_LEDGER_ACCOUNT)
 *  - P1 0x20  → Provide Contact                  (always active, multi-chunk)
 *  - P1 0x21  → Provide Ledger Account Contact   (HAVE_ADDRESS_BOOK_LEDGER_ACCOUNT, multi-chunk)
 *
 * Multi-chunk reassembly
 * ----------------------
 * Sub-commands that carry large payloads (Rename *) split them across several
 * APDUs.  The first chunk (P2=0x00) is prefixed with a 2-byte big-endian total
 * payload length; continuation chunks (P2=0x80) are appended directly.
 * reassemble_chunks() accumulates the data and returns the assembled buffer
 * once complete.  All other sub-commands pass their single buffer through
 * unchanged.
 */

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "os_apdu.h"
#include "os_helpers.h"
#include "os_print.h"
#include "os_utils.h"
#include "address_book.h"
#include "status_words.h"
#include "identity.h"
#ifdef HAVE_ADDRESS_BOOK_LEDGER_ACCOUNT
#include "ledger_account.h"
#endif

#if defined(HAVE_ADDRESS_BOOK)

/* Private defines------------------------------------------------------------*/
#define P1_REGISTER_IDENTITY 0x01
#define P1_EDIT_CONTACT_NAME 0x02
#define P1_EDIT_IDENTIFIER   0x03
#define P1_EDIT_SCOPE        0x04
#define P1_PROVIDE_CONTACT   0x20

#ifdef HAVE_ADDRESS_BOOK_LEDGER_ACCOUNT
#define P1_REGISTER_LEDGER_ACCOUNT        0x11
#define P1_EDIT_LEDGER_ACCOUNT            0x12
#define P1_PROVIDE_LEDGER_ACCOUNT_CONTACT 0x21
#endif

/** P2 value for the first (or only) APDU chunk of a multi-chunk command. */
#define P2_FIRST_CHUNK 0x00
/** P2 value for continuation APDU chunks. */
#define P2_NEXT_CHUNK  0x80

/** Maximum TLV payload the reassembly buffer must hold.
 *
 * Mirrors the OS_IO_BUFFER_SIZE selection from os_io.h: prefer
 * CUSTOM_IO_APDU_BUFFER_SIZE when an app defines it, otherwise fall back to
 * OS_IO_SEPH_BUFFER_SIZE (set in Makefile.defines — see the sizing rationale
 * in address_book.h).
 *
 * Subtract 8 = 3 B (SEPROXYHAL SPI framing) + 5 B (CLA INS P1 P2 Lc).
 * Coherence with OS_IO_SEPH_BUFFER_SIZE is enforced by the _Static_assert in
 * address_book.h. */
#ifdef CUSTOM_IO_APDU_BUFFER_SIZE
#define ADDRESS_BOOK_MAX_CHUNKED_PAYLOAD (CUSTOM_IO_APDU_BUFFER_SIZE - 3 - 5)
#else
#define ADDRESS_BOOK_MAX_CHUNKED_PAYLOAD (OS_IO_SEPH_BUFFER_SIZE - 3 - 5)
#endif

/* Private types, structures, unions -----------------------------------------*/
typedef enum {
    REASSEMBLY_PENDING,   ///< More chunks needed; intermediate SW already sent.
    REASSEMBLY_COMPLETE,  ///< Full payload assembled; *out_buf/*out_len are valid.
    REASSEMBLY_ERROR,     ///< Protocol error; caller must return SWO_INCORRECT_DATA.
} reassembly_status_t;

/* Private variables ---------------------------------------------------------*/
static uint8_t  s_chunk_buf[ADDRESS_BOOK_MAX_CHUNKED_PAYLOAD] = {0};
static uint16_t s_chunk_total                                 = 0;
static uint16_t s_chunk_received                              = 0;

/* Private functions ---------------------------------------------------------*/

/**
 * @brief Reassemble a multi-chunk APDU payload.
 *
 * The first chunk (P2=P2_FIRST_CHUNK) must be prefixed with a 2-byte
 * big-endian total payload length.  Subsequent chunks (P2=P2_NEXT_CHUNK)
 * are appended directly.
 *
 * @param[in]  data     Chunk data (including the 2-byte header on the first chunk)
 * @param[in]  len      Length of this chunk
 * @param[in]  p2       P2_FIRST_CHUNK (0x00) or P2_NEXT_CHUNK (0x80)
 * @param[out] out_buf  Set to the assembled buffer when REASSEMBLY_COMPLETE
 * @param[out] out_len  Set to the assembled length when REASSEMBLY_COMPLETE
 * @return reassembly_status_t
 */
static reassembly_status_t reassemble_chunks(uint8_t  *data,
                                             size_t    len,
                                             uint8_t   p2,
                                             uint8_t **out_buf,
                                             size_t   *out_len)
{
    if (p2 == P2_FIRST_CHUNK) {
        if (len < 2) {
            PRINTF("[Address Book] First chunk too short (no length header)\n");
            return REASSEMBLY_ERROR;
        }
        s_chunk_total    = U2BE(data, 0);
        s_chunk_received = 0;
        data += 2;
        len -= 2;
        if (s_chunk_total == 0 || s_chunk_total > sizeof(s_chunk_buf)) {
            PRINTF("[Address Book] Invalid total length: %u\n", s_chunk_total);
            s_chunk_total = 0;
            return REASSEMBLY_ERROR;
        }
    }
    else {
        if (s_chunk_total == 0) {
            PRINTF("[Address Book] Unexpected continuation chunk\n");
            return REASSEMBLY_ERROR;
        }
    }

    if ((s_chunk_received > s_chunk_total) || (len > (s_chunk_total - s_chunk_received))) {
        PRINTF(
            "[Address Book] Chunk overflow: %u + %zu > %u\n", s_chunk_received, len, s_chunk_total);
        s_chunk_total = 0;
        return REASSEMBLY_ERROR;
    }
    memmove(s_chunk_buf + s_chunk_received, data, len);
    s_chunk_received += (uint16_t) len;

    if (s_chunk_received < s_chunk_total) {
        return REASSEMBLY_PENDING;
    }

    *out_buf      = s_chunk_buf;
    *out_len      = s_chunk_received;
    s_chunk_total = 0;
    return REASSEMBLY_COMPLETE;
}

/* Exported functions --------------------------------------------------------*/
bolos_err_t addr_book_handle_apdu(uint8_t *buffer, size_t buffer_len, uint8_t p1, uint8_t p2)
{
    bolos_err_t err         = SWO_CONDITIONS_NOT_SATISFIED;
    uint8_t    *payload     = NULL;
    size_t      payload_len = 0;

    switch (p1) {
        case P1_REGISTER_IDENTITY:
            err = register_identity(buffer, buffer_len);
            break;

        case P1_EDIT_CONTACT_NAME:
            err = edit_contact_name(buffer, buffer_len);
            break;

        case P1_EDIT_IDENTIFIER:
            switch (reassemble_chunks(buffer, buffer_len, p2, &payload, &payload_len)) {
                case REASSEMBLY_ERROR:
                    err = SWO_INCORRECT_DATA;
                    break;
                case REASSEMBLY_PENDING:
                    err = SWO_SUCCESS;
                    break;
                case REASSEMBLY_COMPLETE:
                    err = edit_identifier(payload, payload_len);
                    break;
            }
            break;

        case P1_EDIT_SCOPE:
            switch (reassemble_chunks(buffer, buffer_len, p2, &payload, &payload_len)) {
                case REASSEMBLY_ERROR:
                    err = SWO_INCORRECT_DATA;
                    break;
                case REASSEMBLY_PENDING:
                    err = SWO_SUCCESS;
                    break;
                case REASSEMBLY_COMPLETE:
                    err = edit_scope(payload, payload_len);
                    break;
            }
            break;

#ifdef HAVE_ADDRESS_BOOK_LEDGER_ACCOUNT
        case P1_REGISTER_LEDGER_ACCOUNT:
            err = register_ledger_account(buffer, buffer_len);
            break;

        case P1_EDIT_LEDGER_ACCOUNT:
            err = edit_ledger_account(buffer, buffer_len);
            break;
#endif  // HAVE_ADDRESS_BOOK_LEDGER_ACCOUNT

        case P1_PROVIDE_CONTACT:
            switch (reassemble_chunks(buffer, buffer_len, p2, &payload, &payload_len)) {
                case REASSEMBLY_ERROR:
                    err = SWO_INCORRECT_DATA;
                    break;
                case REASSEMBLY_PENDING:
                    err = SWO_SUCCESS;
                    break;
                case REASSEMBLY_COMPLETE:
                    err = provide_contact(payload, payload_len);
                    break;
            }
            break;

#ifdef HAVE_ADDRESS_BOOK_LEDGER_ACCOUNT
        case P1_PROVIDE_LEDGER_ACCOUNT_CONTACT:
            switch (reassemble_chunks(buffer, buffer_len, p2, &payload, &payload_len)) {
                case REASSEMBLY_ERROR:
                    err = SWO_INCORRECT_DATA;
                    break;
                case REASSEMBLY_PENDING:
                    err = SWO_SUCCESS;
                    break;
                case REASSEMBLY_COMPLETE:
                    err = provide_ledger_account_contact(payload, payload_len);
                    break;
            }
            break;
#endif  // HAVE_ADDRESS_BOOK_LEDGER_ACCOUNT

        default:
            break;
    }

    return err;
}
#endif  // HAVE_ADDRESS_BOOK
