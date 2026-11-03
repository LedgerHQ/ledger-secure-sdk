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
 *  - P1 0x01  → Register Identity               (always active)
 *  - P1 0x02  → Edit Contact Name               (always active)
 *  - P1 0x03  → Edit Identifier                 (always active)
 *  - P1 0x04  → Edit Scope                      (always active)
 *  - P1 0x11  → Register Ledger Account         (HAVE_ADDRESS_BOOK_LEDGER_ACCOUNT)
 *  - P1 0x12  → Edit Ledger Account             (HAVE_ADDRESS_BOOK_LEDGER_ACCOUNT)
 *  - P1 0x20  → Provide Contact                 (always active)
 *  - P1 0x21  → Provide Ledger Account Contact  (HAVE_ADDRESS_BOOK_LEDGER_ACCOUNT)
 *
 * Chunked transport
 * -----------------
 * Every sub-command uses the same chunked transport, regardless of payload size.
 * The first chunk (P2=0x00) is prefixed with a 2-byte big-endian total payload length.
 * Continuation chunks (P2=0x80) are appended directly.
 * The shared helper apdu_chunks_reassemble() accumulates the data and returns the
 * assembled buffer.
 * Once complete, the handler is then dispatched on P1.
 * A uniform format keeps the protocol consistent and eases future payload-size evolutions.
 */

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "os_apdu.h"
#include "os_helpers.h"
#include "os_print.h"
#include "os_utils.h"
#include "address_book.h"
#include "apdu_chunks.h"
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

/* Exported functions --------------------------------------------------------*/
bolos_err_t addr_book_handle_apdu(uint8_t *buffer, size_t buffer_len, uint8_t p1, uint8_t p2)
{
    bolos_err_t err         = SWO_CONDITIONS_NOT_SATISFIED;
    uint8_t    *payload     = NULL;
    size_t      payload_len = 0;

    // All sub-commands share the same chunked transport: reassemble the full
    // payload first, then dispatch on P1. Intermediate chunks return success
    // (an intermediate SW is sent) and wait for the next chunk.
    switch (apdu_chunks_reassemble(
        APDU_CHUNKS_OWNER_ADDRESS_BOOK, buffer, buffer_len, p2, &payload, &payload_len)) {
        case APDU_CHUNKS_ERROR:
            return SWO_INCORRECT_DATA;
        case APDU_CHUNKS_PENDING:
            return SWO_SUCCESS;
        case APDU_CHUNKS_COMPLETE:
            break;
    }

    switch (p1) {
        case P1_REGISTER_IDENTITY:
            err = register_identity(payload, payload_len);
            break;

        case P1_EDIT_CONTACT_NAME:
            err = edit_contact_name(payload, payload_len);
            break;

        case P1_EDIT_IDENTIFIER:
            err = edit_identifier(payload, payload_len);
            break;

        case P1_EDIT_SCOPE:
            err = edit_scope(payload, payload_len);
            break;

        case P1_PROVIDE_CONTACT:
            err = provide_contact(payload, payload_len);
            break;

#ifdef HAVE_ADDRESS_BOOK_LEDGER_ACCOUNT
        case P1_REGISTER_LEDGER_ACCOUNT:
            err = register_ledger_account(payload, payload_len);
            break;

        case P1_EDIT_LEDGER_ACCOUNT:
            err = edit_ledger_account(payload, payload_len);
            break;

        case P1_PROVIDE_LEDGER_ACCOUNT_CONTACT:
            err = provide_ledger_account_contact(payload, payload_len);
            break;
#endif  // HAVE_ADDRESS_BOOK_LEDGER_ACCOUNT

        default:
            break;
    }

    return err;
}
#endif  // HAVE_ADDRESS_BOOK
