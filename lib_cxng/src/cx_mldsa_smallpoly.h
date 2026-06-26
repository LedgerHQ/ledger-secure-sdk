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
 * @file    cx_mldsa_smallpoly.h
 * @brief   ML-DSA small polynomial type and NTT mod 3329 (low-RAM optimization).
 *
 * Uses the Kyber prime q_small = 3329 for 16-bit NTT multiplications of
 * short polynomials (challenge c, secret vectors s1/s2).
 * Based on: Bos, Renes, Sprenkels - "Dilithium for Memory Constrained Devices"
 * (AFRICACRYPT 2022).
 */

#ifndef CX_MLDSA_SMALLPOLY_H
#define CX_MLDSA_SMALLPOLY_H

#ifdef HAVE_MLDSA_OPTIMIZATION

#include <stdint.h>
#include "lcx_mldsa.h"
#include "cx_mldsa_poly.h"

/** Small NTT modulus (Kyber prime). */
#define MLDSA_SMALL_Q    3329
/** q^{-1} mod 2^16 for Montgomery reduction. */
#define MLDSA_SMALL_QINV (-3327)

/**
 * @brief   Polynomial with MLDSA_N int16_t coefficients (mod 3329).
 *
 * Sufficient for representing secret polynomials with coefficients in [-eta, eta]
 * and the challenge polynomial with TAU coefficients in {-1, +1}.
 */
typedef struct {
    int16_t coeffs[MLDSA_N];
} mldsa_smallpoly;

/**
 * @brief   Forward NTT modulo 3329 in place.
 *
 * @param[in,out] r  Array of MLDSA_N int16_t coefficients.
 */
void MLDSA_SMALLPOLY_ntt(int16_t r[MLDSA_N]);

/**
 * @brief   Inverse NTT modulo 3329 in place with Montgomery factor.
 *
 * @param[in,out] r  Array of MLDSA_N int16_t coefficients.
 */
void MLDSA_SMALLPOLY_invntt_tomont(int16_t r[MLDSA_N]);

/**
 * @brief   Copy a full polynomial into a smallpoly and apply small NTT.
 *
 * The input polynomial must have coefficients small enough to fit int16_t
 * (e.g., challenge polynomial with coefficients in {-1, 0, +1}).
 *
 * @param[out] out  Small polynomial output (NTT domain mod 3329).
 * @param[in]  in   Full polynomial input.
 */
void MLDSA_SMALLPOLY_ntt_copy(mldsa_smallpoly *out, const mldsa_poly *in);

/**
 * @brief   Basemul two small polynomials in NTT domain, then inverse NTT,
 *          storing the result in a full polynomial.
 *
 * Computes r = INTT(a * b) mod 3329, then sign-extends to int32_t.
 *
 * @param[out] r  Output full polynomial.
 * @param[in]  a  First small polynomial (NTT domain).
 * @param[in]  b  Second small polynomial (NTT domain).
 */
void MLDSA_SMALLPOLY_basemul_invntt(mldsa_poly            *r,
                                    const mldsa_smallpoly *a,
                                    const mldsa_smallpoly *b);

/**
 * @brief   Unpack a packed eta polynomial directly into a smallpoly.
 *
 * @param[out] r    Output small polynomial.
 * @param[in]  a    Packed byte array.
 * @param[in]  eta  Range parameter (2 or 4).
 */
void MLDSA_SMALLPOLY_unpack_eta(mldsa_smallpoly *r, const uint8_t *a, uint8_t eta);

#endif /* HAVE_MLDSA_OPTIMIZATION */
#endif /* CX_MLDSA_SMALLPOLY_H */
