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
#define TYPE_EXTERNAL_ADDRESS 0x11
#define EXTERNAL_NAME_LENGTH  33
#define MAX_ADDRESS_LENGTH    64

/* Exported enumerations -----------------------------------------------------*/

/* Exported types, structures, unions ----------------------------------------*/
typedef struct {
    const char          contact_name[EXTERNAL_NAME_LENGTH];
    const char          address_name[EXTERNAL_NAME_LENGTH];
    const char          address_raw[MAX_ADDRESS_LENGTH];
    uint8_t             address_length;
    path_bip32_t        bip32_path;
    uint64_t            chain_id;
    blockchain_family_e blockchain_family;
} external_address_t;

typedef struct {
    char         old_name[EXTERNAL_NAME_LENGTH];
    char         new_name[EXTERNAL_NAME_LENGTH];
    path_bip32_t bip32_path;
} edit_contact_t;

/* Exported macros------------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/

/* Exported functions prototypes--------------------------------------------- */
bolos_err_t add_external_address(uint8_t *buffer_in, size_t buffer_in_length);

// App-specific wrapper (calls edit_contact_impl with app BIP32 paths)
bolos_err_t edit_contact(const uint8_t *buffer_in, size_t buffer_in_length);
