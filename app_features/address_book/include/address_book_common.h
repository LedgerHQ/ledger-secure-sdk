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

#ifndef ADDRESS_BOOK_COMMON_H
#define ADDRESS_BOOK_COMMON_H

#if defined(HAVE_ADDRESS_BOOK)

#include <stdint.h>
#include <stdbool.h>
#include "tlv_library.h"
#include "bip32.h"
#include "lcx_sha256.h"
#include "address_book.h"
#include "identity.h"
#include "ledger_account.h"
#include "nbgl_use_case.h"

bool address_book_handle_derivation_path(const tlv_data_t *data, path_bip32_t *bip32_path);

bool address_book_handle_chain_id(const tlv_data_t *data, uint64_t *chain_id);

bool address_book_handle_blockchain_family(const tlv_data_t *data, blockchain_family_e *family);

bool address_book_handle_printable_string(const tlv_data_t *data,
                                          char             *output_buffer,
                                          size_t            buffer_size);

void address_book_display_review(const nbgl_icon_details_t *icon,
                                 const char                *reviewTitle,
                                 const char                *confirmText,
                                 nbgl_choiceCallback_t      choiceCallback);

void address_book_handle_review_rejected(nbgl_callback_t finalize);

void address_book_finalize_review(bool            success,
                                  const char     *successMsg,
                                  const char     *errorMsg,
                                  nbgl_callback_t finalize);

// Persistent state for the Register Identity flow (lives through the NBGL callback).
typedef struct {
    identity_t identity;
    uint8_t    group_handle[GROUP_HANDLE_SIZE];
    uint8_t    hmac_proof[CX_SHA256_SIZE];
    uint8_t    gid[GID_SIZE];
    bool       active;
} s_register_state_t;

// Union of all mutually-exclusive per-command payload buffers.
// Only one command is processed at a time, so a single allocation covers every flow.
typedef union {
    s_register_state_t    reg;
    edit_contact_name_t   edit_contact_name;
    edit_scope_t          edit_scope;
    edit_identifier_t     edit_identifier;
    identity_t            provide_contact;
    ledger_account_t      ledger_account;
    edit_ledger_account_t edit_ledger_account;
} ab_payload_u;

// Shared UI buffers: all flows use at most 3 tag/value pairs and one list.
typedef struct {
    nbgl_contentTagValue_t     pairs[3];
    nbgl_contentTagValueList_t list;
} ab_ui_t;

extern ab_payload_u g_ab_payload;
extern ab_ui_t      g_ab_ui;

#endif  // HAVE_ADDRESS_BOOK

#endif  // ADDRESS_BOOK_COMMON_H
