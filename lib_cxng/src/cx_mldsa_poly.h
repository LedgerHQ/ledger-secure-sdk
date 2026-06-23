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
#ifndef CX_MLDSA_POLY_H
#define CX_MLDSA_POLY_H

#include "lcx_mldsa.h"

/**
 * @brief   Polynomial with MLDSA_N int32_t coefficients.
 */
typedef struct {
    int32_t coeffs[MLDSA_N];
} mldsa_poly;

#define MLDSA_QINV 58728449   /**< q^{-1} mod 2^32.            */
#define MLDSA_MONT (-4186625) /**< 2^32 mod q (signed).        */

/**
 * @brief   Montgomery reduction: given a 64-bit integer, compute a*q^{-1} mod 2^32.
 *
 * @param[in] a  64-bit input.
 *
 * @return       32-bit reduced value in (-q, q).
 */
int32_t MLDSA_POLY_montgomery_reduce(int64_t a);

/**
 * @brief   Reduce coefficient to representative in about (-6283009, 6283009).
 *
 * @param[in] a  Input coefficient.
 *
 * @return       Reduced coefficient.
 */
int32_t MLDSA_POLY_reduce32(int32_t a);

/**
 * @brief   Adds q if input is negative.
 *
 * @param[in] a  Input coefficient in (-q, q).
 *
 * @return       Coefficient in [0, q).
 */
int32_t MLDSA_POLY_caddq(int32_t a);

/**
 * @brief   Applies reduce32 to all coefficients of a polynomial.
 *
 * @param[in,out] a  Polynomial to reduce.
 */
void MLDSA_POLY_reduce(mldsa_poly *a);

/**
 * @brief   Applies caddq to all coefficients of a polynomial.
 *
 * @param[in,out] a  Polynomial to make positive.
 */
void MLDSA_POLY_caddq_all(mldsa_poly *a);

/**
 * @brief   Adds polynomial b to polynomial a in place.
 *
 * @param[in,out] a  Accumulator polynomial.
 * @param[in]     b  Polynomial to add.
 */
void MLDSA_POLY_add(mldsa_poly *a, const mldsa_poly *b);

/**
 * @brief   Subtracts polynomial b from polynomial a in place.
 *
 * @param[in,out] a  Accumulator polynomial.
 * @param[in]     b  Polynomial to subtract.
 */
void MLDSA_POLY_sub(mldsa_poly *a, const mldsa_poly *b);

/**
 * @brief   Shifts all coefficients left by D bits.
 *
 * @param[in,out] a  Polynomial to shift.
 */
void MLDSA_POLY_shiftl(mldsa_poly *a);

/**
 * @brief   Forward NTT in place.
 *
 * @param[in,out] a  Polynomial to transform.
 */
void MLDSA_POLY_ntt(mldsa_poly *a);

/**
 * @brief   Inverse NTT and multiply by Montgomery factor.
 *
 * @param[in,out] a  Polynomial to inverse-transform.
 */
void MLDSA_POLY_invntt_tomont(mldsa_poly *a);

/**
 * @brief   Pointwise multiplication (Montgomery) with accumulation.
 *
 * @param[in,out] c      Accumulator polynomial.
 * @param[in]     a      First input polynomial.
 * @param[in]     b      Second input polynomial.
 * @param[in]     first  If non-zero, overwrite c; otherwise accumulate.
 */
void MLDSA_POLY_pointwise_montgomery(mldsa_poly       *c,
                                     const mldsa_poly *a,
                                     const mldsa_poly *b,
                                     int               first);

/**
 * @brief   Checks infinity norm of polynomial against bound B.
 *
 * @param[in] a  Polynomial.
 * @param[in] B  Norm bound.
 *
 * @return       0 if all coefficients within bound, 1 otherwise.
 */
int MLDSA_POLY_chknorm(const mldsa_poly *a, int32_t B);

#endif /* CX_MLDSA_POLY_H */
