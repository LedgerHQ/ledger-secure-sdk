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
#include "identity.h"
#include "nbgl_use_case.h"

#ifdef HAVE_ADDRESS_BOOK_LEDGER_ACCOUNT
#include "ledger_account.h"
#endif

/*
The handle functions must be defined by each Coin application implementing the Address Book feature.

These functions will be called by the common code of the Address Book application
in order to validate the content of the payload, or finalize/cleanup the UI flow.
*/

#ifdef HAVE_ADDRESS_BOOK

/*************** Exported functions prototypes: Register Identity **************/

/**
 * @brief Handle called to validate the received Identity.
 *
 * Called after TLV parsing, before displaying to the user.
 * The coin app may perform chain-specific validation of the identifier
 * (e.g. checksum, encoding). Return false to reject and abort the flow.
 *
 * @param[in] params Parsed identity data (contact_name, contact_scope,
 *                   identifier, identifier_len, bip32_path)
 * @return true if the identity is acceptable, false to reject
 */
bool handle_check_identity(identity_t *params);

/**
 * @brief Handle called to finalize the UI flow for registering an Identity.
 *
 * Called after the user has confirmed or rejected the registration.
 * The HMAC proof has already been sent to the host at this point.
 * The app should release any UI resources and return the device to idle.
 */
void finalize_ui_register_identity(void);

/**
 * @brief Callback to retrieve a tag-value pair for the Register Identity UI.
 *
 * Called by NBGL during the review flow to populate the display.
 * Typical mapping: index 0 = contact name, index 1 = identifier (hex or string).
 * Return NULL for out-of-range indices to signal the end of the list.
 *
 * @param[in] pairIndex Index of the tag-value pair to retrieve (0-based).
 * @return Pointer to the tag-value pair, or NULL if pairIndex is out of range.
 */
nbgl_contentTagValue_t *get_register_identity_tagValue(uint8_t pairIndex);

#ifdef HAVE_ADDRESS_BOOK_LEDGER_ACCOUNT

/*************** Exported functions prototypes: Ledger Account *****************/

/**
 * @brief Handle called to validate the received Ledger Account.
 *
 * Called after TLV parsing, before displaying to the user.
 * The coin app must verify that the BIP32 derivation path and chain parameters
 * correspond to a valid account on the chain.
 * Return false to reject and abort the registration flow.
 *
 * @param[in] params Parsed ledger account data (account_name, bip32_path,
 *                   chain_id, blockchain_family)
 * @return true if the account is acceptable, false to reject
 */
bool handle_check_ledger_account(ledger_account_t *params);

/**
 * @brief Callback to retrieve a tag-value pair for the Ledger Account review UI.
 *
 * Called by NBGL during the review flow to populate the display.
 * Typical mapping: index 0 = account name, index 1 = derived address or BIP32 path.
 * Return NULL for out-of-range indices to signal the end of the list.
 *
 * @param[in] pairIndex Index of the tag-value pair to retrieve (0-based).
 * @return Pointer to the tag-value pair, or NULL if pairIndex is out of range.
 */
nbgl_contentTagValue_t *get_register_ledger_account_tagValue(uint8_t pairIndex);

/**
 * @brief Callback to retrieve the coin icon for the Ledger Account review UI.
 *
 * The returned icon is displayed alongside the account details during the review.
 * Return NULL if no icon is available (NBGL will display without an icon).
 *
 * @return Pointer to the icon details structure, or NULL.
 */
const nbgl_icon_details_t *get_ledger_account_icon(void);

/**
 * @brief Handle called to finalize the UI flow for registering a Ledger Account.
 *
 * Called after the user has confirmed or rejected the registration.
 * The HMAC proof has already been sent to the host at this point.
 * The app should release any UI resources and return the device to idle.
 */
void finalize_ui_register_ledger_account(void);

#endif  // HAVE_ADDRESS_BOOK_LEDGER_ACCOUNT

#endif  // HAVE_ADDRESS_BOOK
