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

/*
The handle functions must be defined by each Coin application implementing the Address Book feature.

These functions will be called by the common code of the Address Book application
when processing the APDU, in order to validate the content of the payload.
*/

/* This handle is called to validate the external address.
 *
 * If the address does belong to the device, result is set to 1. Otherwise it
 * is set to 0.
 */
/**
 * @brief Handle called to validate the received external address.
 *
 * @param[in] params buffer_t with the complete TLV payload
 * @return whether the external address is valid or not
 */
bool handle_check_external_address(external_address_t *params);
