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

/*
The handle functions must be defined by each Coin application implementing the Address Book feature.

These functions will be called by the common code of the Address Book application
when processing the APDU, in order to validate the content of the payload.
*/

/**
 * @brief Handle called to validate the received External Address.
 *
 * @param[in] params buffer_t with the complete TLV payload
 * @return whether the external address is valid or not
 */
bool handle_check_external_address(external_address_t *params);

/**
 * @brief Handle called to validate the received Ledger Account.
 *
 * @param[in] params buffer_t with the complete TLV payload
 * @return whether the ledger account is valid or not
 */
bool handle_check_ledger_account(ledger_account_t *params);

/**
 * @brief Handle called to edit a contact name in the address book
 *
 * @param[in] old_contact_name The current name of the contact
 * @param[in] new_contact_name The new name for the contact
 * @return whether the contact name was successfully edited or not
 */
bool handle_edit_contact(const char *old_contact_name, const char *new_contact_name);
