/*******************************************************************************
 *   (c) 2023 Ledger
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

#include "swap_lib_calls.h"

// ################################################################################################

/*
--8<-- [start:coin_api_intro]
The handle functions must be defined by each Coin application implementing the SWAP feature.

Handlers are called by Exchange through the `os_lib_call` API, and their commands must be fulfilled
by the Coin application.

The Exchange application is responsible for handling the flow and sequencing of the SWAP.
--8<-- [end:coin_api_intro]
*/

// ################################################################################################

/*
--8<-- [start:swap_entry_point_intro]
## C applications
For C applications, the lib_standard_app dispatches the command through the `main()` and
`library_app_main()` functions to the Coin application defined handler.
--8<-- [end:swap_entry_point_intro]
*/

// ################################################################################################

/*
--8<-- [start:swap_handle_check_address_brief]
This handle is called when the Exchange application wants to ensure that a
given address belongs to the device.

If the address does belong to the device, result is set to 1. Otherwise it
is set to 0.
--8<-- [end:swap_handle_check_address_brief]
*/

// ################################################################################################

// --8<-- [start:swap_handle_check_address]
// This callback must be defined in Coin Applications that use the SWAP feature.

void swap_handle_check_address(check_address_parameters_t *params);
// --8<-- [end:swap_handle_check_address]

/*
--8<-- [start:swap_handle_get_printable_amount_brief]
This handle is called when the Exchange application wants to format for
display an amount + ticker of a currency known by this application

If the formatting succeeds, result is set to the formatted string. Otherwise
it is set to '\0'.
--8<-- [end:swap_handle_get_printable_amount_brief]
*/

// --8<-- [start:swap_handle_get_printable_amount]
// This callback must be defined in Coin Applications that use the SWAP feature.
void swap_handle_get_printable_amount(get_printable_amount_parameters_t *params);
// --8<-- [end:swap_handle_get_printable_amount]

// --8<-- [start:swap_copy_transaction_parameters]
/* This handle is called when the user has validated on screen the transaction
 * proposal sent by the partner and started the FROM Coin application to sign
 * the payment transaction.
 *
 * This handler needs to save in the heap the details of what has been validated
 * in Exchange. These elements will be checked against the received transaction
 * upon its reception from the Ledger Live.
 *
 * return false on error, true otherwise
 */
bool swap_copy_transaction_parameters(create_transaction_parameters_t *sign_transaction_params);
// --8<-- [end:swap_copy_transaction_parameters]
