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

#ifdef HAVE_ADDRESS_BOOK

bool address_book_compute_hmac_proof_identity(const path_bip32_t *bip32_path,
                                              const char         *name,
                                              const char         *scope,
                                              const uint8_t      *identifier,
                                              uint8_t             identifier_len,
                                              uint8_t             hmac_out[CX_SHA256_SIZE]);

bool address_book_verify_hmac_proof_identity(const path_bip32_t *bip32_path,
                                             const char         *name,
                                             const char         *scope,
                                             const uint8_t      *identifier,
                                             uint8_t             identifier_len,
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

bool address_book_send_hmac_proof(uint8_t type, const uint8_t hmac_proof[CX_SHA256_SIZE]);

#endif  // HAVE_ADDRESS_BOOK
