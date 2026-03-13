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
#include "buffer.h"
#include "lcx_ecfp.h"
#include "ox_aes.h"

#ifdef HAVE_ADDRESS_BOOK

/* Constants -----------------------------------------------------------------*/

// Maximum remaining payload size calculation
// External: type(1) + len(1) + name(33) + len(1) + addr(64) + chain(8) + family(1) = 109
// Ledger:   type(1) + len(1) + path(40) + scheme(1) + chain(8) = 51
#define MAX_EXTERNAL_REMAINING (1 + 1 + EXTERNAL_NAME_LENGTH + 1 + MAX_ADDRESS_LENGTH + 8 + 1)
#define MAX_LEDGER_REMAINING   (1 + 1 + (4 * MAX_BIP32_PATH) + 1 + 8)

/* Functions -----------------------------------------------------------------*/

bool address_book_sign_encrypt_and_send(const uint32_t *derivation_path,
                                        size_t          derivation_path_len,
                                        const uint8_t  *name_payload,
                                        size_t          name_payload_len,
                                        const uint8_t  *remaining_payload,
                                        size_t          remaining_payload_len);

bool address_book_decrypt(const buffer_t *buffer,
                          const uint32_t *encrypt_path,
                          size_t          encrypt_path_len,
                          char           *output_name);

#endif  // HAVE_ADDRESS_BOOK
