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

/** Set to @c true when the application is running as an Exchange library (swap mode). */
extern volatile bool G_called_from_swap;

/** Set to @c true by the coin application when the signing result is ready;
 *  causes @ref io_send_response_buffers() to call @c os_lib_end() instead of returning. */
extern volatile bool G_swap_response_ready;

/** Internal SDK pointer to the @c result field in @ref create_transaction_parameters_t.
 *  Do not use in application code. */
extern volatile uint8_t *G_swap_signing_return_value_address;

bool swap_str_to_u64(const uint8_t *src, size_t length, uint64_t *result);
bool swap_parse_config(const uint8_t *config,
                       uint8_t        config_len,
                       char          *ticker,
                       uint8_t        ticker_buf_len,
                       uint8_t       *decimals);
