/*******************************************************************************
 *   (c) 2026 Ledger SAS
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
 ********************************************************************************/
#pragma once

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include "external_address.h"
#include "ledger_account.h"
#include "buffer.h"
#include "nbgl_use_case.h"

/*
The handle functions must be defined by each Coin application implementing the Address Book feature.

These functions will be called by the common code of the Address Book application
in order to validate the content of the payload, or finalize/cleanup the UI flow.
*/

/*************** Exported functions prototypes: External Address ***************/

/**
 * @brief Handle called to validate the received External Address.
 *
 * @param[in] params buffer_t with the complete TLV payload
 * @return whether the external address is valid or not
 */
bool handle_check_external_address(external_address_t *params);

/**
 * @brief Handle called to finalize the UI flow for registering an External Address.
 */
void finalize_ui_external_address(void);

/**
 * @brief Callback to retrieve the tag-value pair for the External Address UI.
 *
 * @param[in] pairIndex The index of the tag-value pair to retrieve.
 * @return Pointer to the tag-value pair structure, or NULL if the index is invalid.
 */
nbgl_contentTagValue_t *get_external_address_tagValue(uint8_t pairIndex);

/*************** Exported functions prototypes: Edit Contact *******************/

/**
 * @brief Handle called to finalize the UI flow for editing a Contact Name.
 */
void finalize_ui_edit_contact(void);

/*************** Exported functions prototypes: Ledger Account *****************/

/**
 * @brief Handle called to validate the received Ledger Account Address.
 *
 * @param[in] params buffer_t with the complete TLV payload
 * @return whether the ledger account is valid or not
 */
bool handle_check_ledger_account(ledger_account_t *params);

/**
 * @brief Callback to retrieve the tag-value pair for the Ledger Account UI.
 *
 * @param[in] pairIndex The index of the tag-value pair to retrieve.
 * @return Pointer to the tag-value pair structure, or NULL if the index is invalid.
 */
nbgl_contentTagValue_t *get_ledger_account_tagValue(uint8_t pairIndex);

/**
 * @brief Callback to retrieve the icon for the Ledger Account UI.
 *
 * @return Pointer to the icon details structure, or NULL if no icon is available.
 */
const nbgl_icon_details_t *get_ledger_account_icon(void);

/**
 * @brief Handle called to finalize the UI flow for registering a Ledger Account.
 */
void finalize_ui_ledger_account(void);
