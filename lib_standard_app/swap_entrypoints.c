/*******************************************************************************
 *   (c) 2026 Ledger
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

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "swap_entrypoints.h"
#include "macros.h"
#include "os.h"
#include "chain_config.h"
#include "swap_utils.h"
#include "main_std_app.h"
#ifdef HAVE_NBGL
#include "nbgl_use_case.h"
#endif  // HAVE_NBGL

#ifdef HAVE_SWAP

/** @brief Entrypoint called by the Exchange application to ensure
 * a given address belongs to the device.
 * This is the weak implementation, meant to be overridden by the Coin application.
 *
 * @param[in] params Pointer to the parameters of the check address request
 * @param[in] chain_config Pointer to the chain configuration
 *
 * @note The result is set to 1 if the address belongs to the device, else to 0.
 */
WEAK void swap_handle_check_address_ext(check_address_parameters_t *params,
                                        const coin_chain_config_t  *chain_config)
{
    UNUSED(chain_config);
    swap_handle_check_address(params);
}

/** @brief Entrypoint called by the Exchange application to format an amount + ticker
 * of a currency known by this application.
 * This is the weak implementation, meant to be overridden by the Coin application.
 *
 * @param[in] params Pointer to the parameters of the get printable amount request
 * @param[in] chain_config Pointer to the chain configuration
 *
 * @note If the formatting succeeds, result is set to the formatted string, else to '\0'.
 */
WEAK void swap_handle_get_printable_amount_ext(get_printable_amount_parameters_t *params,
                                               const coin_chain_config_t         *chain_config)
{
    UNUSED(chain_config);
    swap_handle_get_printable_amount(params);
}

/** @brief Entrypoint called by the Exchange application when the user has validated
 * on screen the transaction proposal sent by the partner
 * and started the FROM Coin application to sign the payment transaction.
 *
 * @param[in] params Pointer to the parameters of the sign transaction request
 * @param[in] chain_config Pointer to the chain configuration
 *
 * @note This handler needs to save in the heap the details of what has been validated
 * in Exchange. These elements will be checked against the received transaction
 * upon its reception from the Ledger Live.
 *
 * @return false on error, true otherwise
 */
WEAK bool swap_copy_transaction_parameters_ext(create_transaction_parameters_t *params,
                                               const coin_chain_config_t       *chain_config)
{
    UNUSED(chain_config);
    return swap_copy_transaction_parameters(params);
}

/**
 * @brief Entrypoint called to sign a transaction requested by Exchange
 * This function initializes the application in swap mode and starts the main application loop.
 * This is the weak implementation, meant to be overridden by the Coin application.
 *
 * @param[in] params Pointer to the parameters of the sign transaction request
 */
WEAK void NORETURN swap_handle_sign_transaction(create_transaction_parameters_t *params)
{
    common_app_init();
    common_app_setup(true);

    // BSS was wiped, we can now init these globals
    G_called_from_swap    = true;
    G_swap_response_ready = false;
    // Keep the address at which we'll reply the signing status
    G_swap_signing_return_value_address = &params->result;

#ifdef HAVE_NBGL
    nbgl_useCaseSpinner("Signing");
#endif  // HAVE_NBGL

    app_main();

    // remove the warning caused by -Winvalid-noreturn
    __builtin_unreachable();
}

/**
 * @brief Entrypoint called to sign a transaction requested by Exchange
 * This function initializes the application in swap mode and starts the main application loop.
 * This is the weak implementation, meant to be overridden by the Coin application.
 *
 * @param[in] params Pointer to the parameters of the sign transaction request
 * @param[in] config Pointer to the chain configuration
 */
WEAK void NORETURN swap_handle_sign_transaction_ext(create_transaction_parameters_t *params,
                                                    const coin_chain_config_t       *config)
{
    if (config != NULL) {
        coin_chain_config = config;
    }
    swap_handle_sign_transaction(params);
}

#endif  // HAVE_SWAP
