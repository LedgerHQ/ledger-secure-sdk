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
#define TYPE_REGISTER_LEDGER_ACCOUNT        0x2f
#define TYPE_EDIT_LEDGER_ACCOUNT            0x30
#define TYPE_PROVIDE_LEDGER_ACCOUNT_CONTACT 0x34
#define ACCOUNT_NAME_LENGTH                 33  ///< max 32 printable chars + null terminator

/* Exported types, structures, unions ----------------------------------------*/

/**
 * @brief Data extracted from a Register Ledger Account TLV payload.
 */
typedef struct {
    char                account_name[ACCOUNT_NAME_LENGTH];
    path_bip32_t        bip32_path;
    uint64_t            chain_id;
    blockchain_family_e blockchain_family;
} ledger_account_t;

/**
 * @brief Data extracted from a Rename Ledger Account TLV payload.
 */
typedef struct {
    ledger_account_t ledger_account;  ///< New account (account_name = new name)
    char             previous_account_name[ACCOUNT_NAME_LENGTH];  ///< Name being replaced
} edit_ledger_account_t;

/* Exported macros------------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/

/* Exported functions prototypes--------------------------------------------- */
bolos_err_t register_ledger_account(uint8_t *buffer_in, size_t buffer_in_length);
bolos_err_t edit_ledger_account(uint8_t *buffer_in, size_t buffer_in_length);
bolos_err_t provide_ledger_account_contact(uint8_t *buffer_in, size_t buffer_in_length);

/**
 * @brief Return a read-only pointer to the parsed Edit Ledger Account data.
 *
 * Valid from the moment edit_ledger_account() has finished parsing until the
 * next call to edit_ledger_account(), which overwrites the static buffer.
 *
 * @return Pointer to the current EDIT_LEDGER_ACCOUNT static (never NULL)
 */
const edit_ledger_account_t *get_edit_ledger_account(void);
