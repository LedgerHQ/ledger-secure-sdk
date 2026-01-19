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
#ifdef HAVE_SWAP

#include <stddef.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "macros.h"
#include "chain_config.h"

volatile bool G_called_from_swap;
volatile bool G_swap_response_ready;
// Save the BSS address where we will write the return value when finished
// Don't use in application, SDK only
volatile uint8_t *G_swap_signing_return_value_address;

bool swap_str_to_u64(const uint8_t *src, size_t length, uint64_t *result)
{
    if (length > sizeof(uint64_t)) {
        return false;
    }
    uint64_t value = 0;
    for (size_t i = 0; i < length; i++) {
        value <<= 8;
        value |= src[i];
    }
    *result = value;
    return true;
}

bool swap_parse_config(const uint8_t *config,
                       uint8_t        config_len,
                       char          *ticker,
                       uint8_t        ticker_buf_len,
                       uint8_t       *decimals)
{
    uint8_t ticker_len, offset = 0;
    if (config_len == 0 || config == NULL) {
        return false;
    }
    ticker_len = config[offset++];
    if (ticker_len == 0 || ticker_len >= ticker_buf_len - 1 || config_len - offset < ticker_len) {
        return false;
    }
    memcpy(ticker, config + offset, ticker_len);
    offset += ticker_len;
    ticker[ticker_len] = '\0';

    if (config_len - offset < 1) {
        return false;
    }
    *decimals = config[offset];
    return true;
}

/** @brief Initialize the chain configuration with default values
 *
 * @param[out] coin_config Pointer to the chain configuration to initialize
 */
WEAK void swap_init_coin_config(coin_chain_config_t *coin_config)
{
    memset(coin_config, 0, sizeof(coin_chain_config_t));
#if defined(APP_TICKER) && defined(APP_CHAIN_ID)
    _Static_assert(strlen(APP_TICKER) <= sizeof(coin_config->coinName),
                   "APP_TICKER is too long for coinName buffer");
    strncpy(coin_config->coinName, APP_TICKER, sizeof(coin_config->coinName) - 1);
    coin_config->chainId = APP_CHAIN_ID;
#endif  // defined(APP_TICKER) && defined(APP_CHAIN_ID)
}

#endif  // HAVE_SWAP
