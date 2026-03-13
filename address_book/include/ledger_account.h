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

/* Includes ------------------------------------------------------------------*/
#include "os_types.h"
#include "address_book.h"
#include "bip32.h"

/* Exported defines   --------------------------------------------------------*/
#define TYPE_REGISTER_LEDGER_ACCOUNT 0x13
#define TYPE_RENAME_LEDGER_ACCOUNT   0x14
#define ACCOUNT_NAME_LENGTH          33  ///< max 32 printable chars + null terminator

/* Exported types, structures, unions ----------------------------------------*/
typedef struct {
    const char          account_name[ACCOUNT_NAME_LENGTH];
    path_bip32_t        bip32_path;
    uint64_t            chain_id;
    blockchain_family_e blockchain_family;
} ledger_account_t;

/* Exported macros------------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/

/* Exported functions prototypes--------------------------------------------- */
bolos_err_t register_ledger_account(uint8_t *buffer_in, size_t buffer_in_length);
