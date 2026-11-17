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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "transaction_check.h"

/*
The handle functions must be defined by each Coin application implementing the Transaction Check
feature. They hold what the SDK cannot know: the chain-specific checks, the persistent settings
and the app UI states.
*/

#if defined(HAVE_TX_CHECK)

/**
 * @brief Validate the chain-specific fields of a received descriptor.
 *
 * Called after the TLV parsing and the signature verification, before the descriptor is stored.
 * Typically checks the address size and the supported check types.
 *
 * @param[in] descriptor Parsed descriptor (zero-copy buffers valid only during the call)
 * @return true to store the descriptor, false to reject it
 */
bool handle_check_tx_report(const tlv_transaction_check_out_t *descriptor);

/**
 * @brief Check that the report address is the signer of the operation.
 *
 * @param[in] address     Address received in the descriptor, in the chain format
 * @param[in] address_len Address length
 * @param[in] ctx         Operation being signed
 * @return true if the address is the signer
 */
bool handle_check_tx_report_signer(const uint8_t            *address,
                                   size_t                    address_len,
                                   const tx_check_context_t *ctx);

/**
 * @brief Check the report chain ID against the operation being signed.
 *
 * @param[in] chain_id_received Whether the descriptor holds a chain ID
 * @param[in] chain_id          Chain ID of the descriptor
 * @param[in] ctx               Operation being signed
 * @return true if the report applies to this chain
 */
bool handle_check_tx_report_chain_id(bool                      chain_id_received,
                                     uint64_t                  chain_id,
                                     const tx_check_context_t *ctx);

/**
 * @brief Read a persistent Transaction Check setting.
 *
 * @param[in] setting Setting to read
 * @return Setting value
 */
bool handle_tx_check_get_setting(tx_check_setting_t setting);

/**
 * @brief Write a persistent Transaction Check setting.
 *
 * @param[in] setting Setting to write
 * @param[in] value   New value
 */
void handle_tx_check_set_setting(tx_check_setting_t setting, bool value);

/**
 * @brief Tell whether the app is idle, so that the opt-in screen can be displayed.
 *
 * @return true if no flow is on screen
 */
bool handle_tx_check_is_idle(void);

/**
 * @brief Reply to an opt-in request received on the app APDU.
 *
 * Called once the user has chosen, before finalize_ui_tx_check_opt_in(), when tx_check_opt_in()
 * was called with a pending reply. Optional: the SDK default sends the 1-byte enable state with
 * SWO_SUCCESS.
 *
 * @param[in] enabled Transaction Check enable state, to send as the 1-byte reply
 */
void handle_tx_check_opt_in_response(bool enabled);

/**
 * @brief Leave the opt-in flow.
 *
 * Called once the user has chosen; the APDU reply, if any, has already been sent.
 *
 * @param[in] response_expected Whether the flow was started by an APDU (else from the settings)
 */
void finalize_ui_tx_check_opt_in(bool response_expected);

#endif  // HAVE_TX_CHECK
