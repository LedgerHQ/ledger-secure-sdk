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

#include <stdint.h>
#include <stdbool.h>
#include "tlv_library.h"
#include "bip32.h"
#include "address_book.h"

bool address_book_handle_derivation_path(const tlv_data_t *data, path_bip32_t *bip32_path);

bool address_book_handle_chain_id(const tlv_data_t *data, uint64_t *chain_id);

bool address_book_handle_blockchain_family(const tlv_data_t *data, blockchain_family_e *family);

bool address_book_handle_printable_string(const tlv_data_t *data,
                                          char             *output_buffer,
                                          size_t            buffer_size);

#endif  // ADDRESS_BOOK_COMMON_H
