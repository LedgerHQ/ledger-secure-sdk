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
#ifndef CX_MLDSA_SAMPLE_H
#define CX_MLDSA_SAMPLE_H

#include "cx_mldsa_poly.h"
#include "lcx_mldsa.h"

/**
 * @brief   Sample polynomial with uniformly random coefficients in [0, q-1]
 *          by performing rejection sampling on output stream of SHAKE128.
 *
 * @param[out] a     Output polynomial.
 * @param[in]  seed  Byte array with seed of MLDSA_SEEDBYTES bytes.
 * @param[in]  nonce Two-byte nonce.
 */
void MLDSA_SAMPLE_uniform(mldsa_poly *a, const uint8_t seed[MLDSA_SEEDBYTES], uint16_t nonce);

/**
 * @brief   Sample polynomial with coefficients in [-eta, eta] from
 *          SHAKE256(seed||nonce).
 *
 * @param[out] a     Output polynomial.
 * @param[in]  seed  Byte array with seed of MLDSA_CRHBYTES bytes.
 * @param[in]  nonce Single-byte nonce.
 * @param[in]  eta   Range parameter (2 or 4).
 */
void MLDSA_SAMPLE_eta(mldsa_poly   *a,
                      const uint8_t seed[MLDSA_CRHBYTES],
                      uint16_t      nonce,
                      uint8_t       eta);

/**
 * @brief   Sample polynomial with coefficients in [-(gamma1-1), gamma1]
 *          from SHAKE256(seed||nonce).
 *
 * @param[out] a       Output polynomial.
 * @param[in]  seed    Byte array with seed of MLDSA_CRHBYTES bytes.
 * @param[in]  nonce   16-bit nonce.
 * @param[in]  gamma1  Coefficient range parameter.
 */
void MLDSA_SAMPLE_gamma1(mldsa_poly   *a,
                         const uint8_t seed[MLDSA_CRHBYTES],
                         uint16_t      nonce,
                         int32_t       gamma1);

/**
 * @brief   Sample challenge polynomial with TAU coefficients in {-1, +1}.
 *
 * @param[out] c     Output polynomial (TAU nonzero coefficients).
 * @param[in]  seed  Challenge hash of ctilde_bytes length.
 * @param[in]  seedlen Length of seed.
 * @param[in]  tau   Number of nonzero coefficients.
 */
void MLDSA_SAMPLE_challenge(mldsa_poly *c, const uint8_t *seed, size_t seedlen, uint8_t tau);

#endif /* CX_MLDSA_SAMPLE_H */
