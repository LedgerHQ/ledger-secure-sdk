
/*******************************************************************************
 *   Ledger Nano S - Secure firmware
 *   (c) 2022 Ledger
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

#ifndef CX_RNG_H
#define CX_RNG_H

#ifdef BOLOS_OS_UPGRADER_APP
#include "osu_defines.h"
#endif

#ifdef HAVE_RNG

#include <stdint.h>

/// @brief function to be called each time random data is needed
uint32_t cx_trng_u32(void);

#endif  // HAVE_RNG

#endif  // CX_RNG_G
