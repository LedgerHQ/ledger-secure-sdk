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
#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "bip32.h"
#include "lcx_sha256.h"
#include "address_book.h"
#include "identity.h"

#ifdef HAVE_ADDRESS_BOOK

bool address_book_generate_group_handle(const path_bip32_t *bip32_path,
                                        uint8_t             group_handle[GROUP_HANDLE_SIZE]);

bool address_book_verify_group_handle(const path_bip32_t *bip32_path,
                                      const uint8_t       group_handle[GROUP_HANDLE_SIZE],
                                      uint8_t             gid_out[GID_SIZE]);

bool address_book_compute_hmac_name(const path_bip32_t *bip32_path,
                                    const uint8_t       gid[GID_SIZE],
                                    const char         *name,
                                    uint8_t             hmac_out[CX_SHA256_SIZE]);

bool address_book_verify_hmac_name(const path_bip32_t *bip32_path,
                                   const uint8_t       gid[GID_SIZE],
                                   const char         *name,
                                   const uint8_t       hmac_expected[CX_SHA256_SIZE]);

bool address_book_compute_hmac_rest(const path_bip32_t *bip32_path,
                                    const uint8_t       gid[GID_SIZE],
                                    const char         *scope,
                                    const uint8_t      *identifier,
                                    uint8_t             identifier_len,
                                    blockchain_family_e family,
                                    uint64_t            chain_id,
                                    uint8_t             hmac_out[CX_SHA256_SIZE]);

bool address_book_verify_hmac_rest(const path_bip32_t *bip32_path,
                                   const uint8_t       gid[GID_SIZE],
                                   const char         *scope,
                                   const uint8_t      *identifier,
                                   uint8_t             identifier_len,
                                   blockchain_family_e family,
                                   uint64_t            chain_id,
                                   const uint8_t       hmac_expected[CX_SHA256_SIZE]);

#ifdef HAVE_ADDRESS_BOOK_LEDGER_ACCOUNT

bool address_book_compute_hmac_proof_ledger_account(const path_bip32_t *bip32_path,
                                                    const char         *name,
                                                    blockchain_family_e family,
                                                    uint64_t            chain_id,
                                                    uint8_t             hmac_out[CX_SHA256_SIZE]);

bool address_book_verify_hmac_proof_ledger_account(const path_bip32_t *bip32_path,
                                                   const char         *name,
                                                   blockchain_family_e family,
                                                   uint64_t            chain_id,
                                                   const uint8_t hmac_expected[CX_SHA256_SIZE]);

#endif  // HAVE_ADDRESS_BOOK_LEDGER_ACCOUNT

/** type(1) | hmac(32) — for Edit operations. */
bool address_book_send_hmac_proof(uint8_t type, const uint8_t hmac_proof[CX_SHA256_SIZE]);

/** type(1) | group_handle(64) | hmac_name(32) | hmac_rest(32) = 129 B — for Register Identity. */
bool address_book_send_register_identity_response(const uint8_t group_handle[GROUP_HANDLE_SIZE],
                                                  const uint8_t hmac_name[CX_SHA256_SIZE],
                                                  const uint8_t hmac_rest[CX_SHA256_SIZE]);

#endif  // HAVE_ADDRESS_BOOK
