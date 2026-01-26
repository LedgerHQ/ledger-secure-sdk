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

#pragma once

#include <stdint.h>
#include "tokens.h"

typedef struct {
    char     coinName[MAX_TICKER_SIZE + 1];  // ticker + '\0'
    uint64_t chainId;
} coin_chain_config_t;

extern const coin_chain_config_t *coin_chain_config;
