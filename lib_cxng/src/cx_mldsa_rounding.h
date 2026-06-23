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
#ifndef CX_MLDSA_ROUNDING_H
#define CX_MLDSA_ROUNDING_H

#include "cx_mldsa_poly.h"
#include "lcx_mldsa.h"

/**
 * @brief   For coefficient a, compute high and low bits a0, a1 such that
 *          a mod q = a1*2^D + a0, with -2^{D-1} < a0 <= 2^{D-1}.
 *
 * @param[in]  a   Coefficient.
 * @param[out] a0  Low bits output.
 *
 * @return         High bits a1.
 */
int32_t MLDSA_ROUNDING_power2round(int32_t a, int32_t *a0);

/**
 * @brief   For coefficient a, compute high and low bits a0, a1 such that
 *          a mod q = a1*ALPHA + a0, where ALPHA = 2*gamma2.
 *
 * @param[in]  a       Coefficient (standard representative).
 * @param[out] a0      Low bits output.
 * @param[in]  gamma2  Low-order rounding range.
 *
 * @return             High bits a1.
 */
int32_t MLDSA_ROUNDING_decompose(int32_t a, int32_t *a0, int32_t gamma2);

/**
 * @brief   Compute hint bit. Returns 1 if adding ct0 to w - ct0 does
 *          not change the high bits.
 *
 * @param[in] a0      Low part.
 * @param[in] a1      High part.
 * @param[in] gamma2  Low-order rounding range.
 *
 * @return            Hint bit (0 or 1).
 */
uint32_t MLDSA_ROUNDING_make_hint(int32_t a0, int32_t a1, int32_t gamma2);

/**
 * @brief   Correct high bits using hint.
 *
 * @param[in] a       Coefficient.
 * @param[in] hint    Hint bit (0 or 1).
 * @param[in] gamma2  Low-order rounding range.
 *
 * @return            Corrected high bits.
 */
int32_t MLDSA_ROUNDING_use_hint(int32_t a, uint32_t hint, int32_t gamma2);

/**
 * @brief   Applies power2round to all coefficients of a polynomial.
 *
 * @param[out] a1  High bits output polynomial.
 * @param[out] a0  Low bits output polynomial.
 * @param[in]  a   Input polynomial.
 */
void MLDSA_ROUNDING_poly_power2round(mldsa_poly *a1, mldsa_poly *a0, const mldsa_poly *a);

/**
 * @brief   Applies decompose to all coefficients of a polynomial.
 *
 * @param[out] a1      High bits output polynomial.
 * @param[out] a0      Low bits output polynomial.
 * @param[in]  a       Input polynomial.
 * @param[in]  gamma2  Low-order rounding range.
 */
void MLDSA_ROUNDING_poly_decompose(mldsa_poly       *a1,
                                   mldsa_poly       *a0,
                                   const mldsa_poly *a,
                                   int32_t           gamma2);

/**
 * @brief   Applies use_hint to all coefficients of a polynomial.
 *
 * @param[out] b       Output polynomial.
 * @param[in]  a       Input polynomial.
 * @param[in]  h       Hint polynomial.
 * @param[in]  gamma2  Low-order rounding range.
 */
void MLDSA_ROUNDING_poly_use_hint(mldsa_poly       *b,
                                  const mldsa_poly *a,
                                  const mldsa_poly *h,
                                  int32_t           gamma2);

#endif /* CX_MLDSA_ROUNDING_H */
