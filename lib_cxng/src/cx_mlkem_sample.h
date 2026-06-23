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
#ifndef CX_MLKEM_SAMPLE_H
#define CX_MLKEM_SAMPLE_H

#include <stdint.h>
#include "cx_errors.h"
#include "lcx_mlkem.h"

/**
 * @brief   Samples a polynomial using the centered binomial distribution with eta=2.
 *
 * @param[out] r    Output polynomial.
 * @param[in]  buf  Input byte buffer of 2 * MLKEM_N / 4 bytes.
 */
void MLKEM_SAMPLE_cbd2(poly *r, const uint8_t *buf);

/**
 * @brief   Samples a polynomial using the centered binomial distribution with eta=3.
 *
 * @param[out] r    Output polynomial.
 * @param[in]  buf  Input byte buffer of 3 * MLKEM_N / 4 bytes.
 */
void MLKEM_SAMPLE_cbd3(poly *r, const uint8_t *buf);

/**
 * @brief   Samples a noise polynomial using PRF and CBD.
 *
 * @param[out] r      Output polynomial.
 * @param[in]  seed   Seed of MLKEM_SYMBYTES bytes.
 * @param[in]  nonce  Nonce byte.
 * @param[in]  eta    CBD parameter (2 or 3).
 */
void MLKEM_SAMPLE_getnoise(poly *r, const uint8_t *seed, uint8_t nonce, uint8_t eta);

/**
 * @brief   Samples a polynomial using rejection sampling from a uniform distribution.
 *
 * @param[out] r     Output polynomial with coefficients in [0, q-1].
 * @param[in]  seed  Seed of MLKEM_SYMBYTES bytes.
 * @param[in]  x     First index byte for the XOF.
 * @param[in]  y     Second index byte for the XOF.
 */
void MLKEM_SAMPLE_rej_uniform(poly *r, const uint8_t *seed, uint8_t x, uint8_t y);
#endif  // CX_MLKEM_SAMPLE_H
