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

/**
 * @file transaction_check_internal.h
 * @brief Transaction Check functions shared between the SDK sources, not exposed to the apps.
 */

#pragma once

#include <stdbool.h>
#include "transaction_check.h"

#if defined(HAVE_SWAP)
#include "swap_utils.h"
#endif

#if defined(HAVE_TX_CHECK)

/**
 * @brief Tell whether the app runs a swap started by the Exchange app.
 */
static inline bool tx_check_in_swap(void)
{
#if defined(HAVE_SWAP)
    return G_called_from_swap;
#else
    return false;
#endif
}

#if defined(HAVE_TX_CHECK_APDU)
/**
 * @brief Opt-in request received on the SDK APDU, whose reply is sent by the SDK.
 *
 * @return Same as tx_check_opt_in()
 */
tx_check_status_t tx_check_opt_in_sdk_apdu(void);
#endif  // HAVE_TX_CHECK_APDU

#endif  // HAVE_TX_CHECK
