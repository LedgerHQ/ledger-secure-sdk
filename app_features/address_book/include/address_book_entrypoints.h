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
 * @param[in] params Parsed identity data (contact_name, scope,
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

/*************** Exported functions prototypes: Edit Identifier ***************/

/**
 * @brief Handle called to validate and store the Edit Identifier parameters.
 *
 * Called after TLV parsing and HMAC proof verification, before displaying
 * the review UI. The coin app should validate the new identifier (e.g.
 * checksum, encoding) and copy the data it needs for display.
 * Return false to reject and abort the flow.
 *
 * @param[in] params Contact name, previous identifier, and new identity data
 * @return true if the edit is acceptable, false to reject
 */
bool handle_check_edit_identifier(const edit_identifier_t *params);

/**
 * @brief Callback to retrieve a tag-value pair for the Edit Identifier UI.
 *
 * Called by NBGL during the review flow to populate the display.
 * Typical mapping:
 *  - index 0: "Contact name" → contact name (unchanged)
 *  - index 1: "Old address"  → previous identifier (hex)
 *  - index 2: "New address"  → new identifier (hex)
 *  - index 3: "Scope"        → contact scope (if non-empty)
 * Return NULL for out-of-range indices.
 *
 * @param[in] pairIndex Index of the tag-value pair to retrieve (0-based).
 * @return Pointer to the tag-value pair, or NULL if pairIndex is out of range.
 */
nbgl_contentTagValue_t *get_edit_identifier_tagValue(uint8_t pairIndex);

/**
 * @brief Handle called to finalize the UI flow for editing an Identifier.
 *
 * Called after the user has confirmed or rejected the edit.
 * The new HMAC proof has already been sent to the host at this point.
 * The app should release any UI resources and return the device to idle.
 */
void finalize_ui_edit_identifier(void);

/*************** Exported functions prototypes: Edit Contact Name **************/

/**
 * @brief Handle called to finalize the UI flow for editing a contact name.
 *
 * Called after the user has confirmed or rejected the edit.
 * The new HMAC proof has already been sent to the host at this point.
 * The app should release any UI resources and return the device to idle.
 */
void finalize_ui_edit_contact_name(void);

/*************** Exported functions prototypes: Edit Scope ****************/

/**
 * @brief Handle called to finalize the UI flow for editing a scope.
 *
 * Called after the user has confirmed or rejected the edit.
 * The new HMAC proof has already been sent to the host at this point.
 * The app should release any UI resources and return the device to idle.
 */
void finalize_ui_edit_scope(void);

/*************** Exported functions prototypes: Provide Contact ***********/

/**
 * @brief Handle called to store a verified contact.
 *
 * Called after TLV parsing and full HMAC verification (group_handle,
 * HMAC_PROOF, HMAC_REST). No UI is shown; the app must store the contact
 * for later use (e.g. address substitution during transaction review).
 * Return false to reject the contact and abort the command with an error.
 *
 * @param[in] contact Fully verified identity (name, scope, identifier,
 *                    identifier_len, blockchain_family, chain_id)
 * @return true if the contact was accepted and stored, false to reject
 */
bool handle_provide_contact(const identity_t *contact);

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
 * @brief Handle called to display the Ledger Account registration review screen.
 *
 * Called after handle_check_ledger_account() succeeds. The app must display
 * the review UI and invoke @p choice_callback on confirmation or rejection.
 * The app owns the UI flow from this point and is responsible for adapting
 * the display to the target device (touch vs Nano).
 */
void display_register_ledger_account_review(nbgl_choiceCallback_t choice_callback);

/**
 * @brief Handle called to finalize the UI flow for registering a Ledger Account.
 *
 * Called after the user has confirmed or rejected the registration.
 * The HMAC proof has already been sent to the host at this point.
 * The app should release any UI resources and return the device to idle.
 */
void finalize_ui_register_ledger_account(void);

/*************** Exported functions prototypes: Edit Ledger Account **********/

/**
 * @brief Handle called to finalize the UI flow for editing a Ledger Account.
 *
 * Called after the user has confirmed or rejected the edit.
 * The new HMAC proof has already been sent to the host at this point.
 * The app should release any UI resources and return the device to idle.
 */
void finalize_ui_edit_ledger_account(void);

/*************** Exported functions prototypes: Provide Ledger Account Contact */

/**
 * @brief Handle called to store a verified Ledger Account contact.
 *
 * Called after TLV parsing and HMAC Proof of Registration verification.
 * No UI is shown; the app must derive the address from the BIP32 path and
 * store the contact for later use (e.g. address substitution during
 * transaction review). Return false to reject and abort with an error.
 *
 * @param[in] account Verified Ledger Account (account_name, bip32_path,
 *                    chain_id, blockchain_family)
 * @return true if the contact was accepted and stored, false to reject
 */
bool handle_provide_ledger_account_contact(const ledger_account_t *account);

#endif  // HAVE_ADDRESS_BOOK_LEDGER_ACCOUNT

#endif  // HAVE_ADDRESS_BOOK
