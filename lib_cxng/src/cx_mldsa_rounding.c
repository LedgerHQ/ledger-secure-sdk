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
 * @file    cx_mldsa_rounding.c
 * @brief   ML-DSA rounding operations (FIPS 204).
 */

#include "cx_mldsa_rounding.h"

int32_t MLDSA_ROUNDING_power2round(int32_t a, int32_t *a0)
{
    int32_t a1;
    a1  = (a + (1 << (MLDSA_D - 1)) - 1) >> MLDSA_D;
    *a0 = a - (a1 << MLDSA_D);
    return a1;
}

int32_t MLDSA_ROUNDING_decompose(int32_t a, int32_t *a0, int32_t gamma2)
{
    int32_t a1;
    a1 = (a + 127) >> 7;

    if (gamma2 == (MLDSA_Q - 1) / 32) {
        a1 = (a1 * 1025 + (1 << 21)) >> 22;
        a1 &= 15;
    }
    else {
        // gamma2 == (MLDSA_Q - 1) / 88
        a1 = (a1 * 11275 + (1 << 23)) >> 24;
        a1 ^= ((43 - a1) >> 31) & a1;
    }

    *a0 = a - a1 * 2 * gamma2;
    *a0 -= (((MLDSA_Q - 1) / 2 - *a0) >> 31) & MLDSA_Q;
    return a1;
}

uint32_t MLDSA_ROUNDING_make_hint(int32_t a0, int32_t a1, int32_t gamma2)
{
    if ((a0 > gamma2) || (a0 < -gamma2) || ((a0 == -gamma2) && (a1 != 0))) {
        return 1U;
    }
    return 0U;
}

int32_t MLDSA_ROUNDING_use_hint(int32_t a, uint32_t hint, int32_t gamma2)
{
    int32_t a0, a1;
    a1 = MLDSA_ROUNDING_decompose(a, &a0, gamma2);

    if (hint == 0U) {
        return a1;
    }

    if (gamma2 == (MLDSA_Q - 1) / 32) {
        if (a0 > 0) {
            return (a1 + 1) & 15;
        }
        else {
            return (a1 - 1) & 15;
        }
    }
    else {
        // gamma2 == (MLDSA_Q - 1) / 88
        if (a0 > 0) {
            return (a1 == 43) ? 0 : a1 + 1;
        }
        else {
            return (a1 == 0) ? 43 : a1 - 1;
        }
    }
}

void MLDSA_ROUNDING_poly_power2round(mldsa_poly *a1, mldsa_poly *a0, const mldsa_poly *a)
{
    for (uint32_t i = 0U; i < MLDSA_N; i++) {
        a1->coeffs[i] = MLDSA_ROUNDING_power2round(a->coeffs[i], &a0->coeffs[i]);
    }
}

void MLDSA_ROUNDING_poly_decompose(mldsa_poly       *a1,
                                   mldsa_poly       *a0,
                                   const mldsa_poly *a,
                                   int32_t           gamma2)
{
    for (uint32_t i = 0U; i < MLDSA_N; i++) {
        a1->coeffs[i] = MLDSA_ROUNDING_decompose(a->coeffs[i], &a0->coeffs[i], gamma2);
    }
}

void MLDSA_ROUNDING_poly_use_hint(mldsa_poly       *b,
                                  const mldsa_poly *a,
                                  const mldsa_poly *h,
                                  int32_t           gamma2)
{
    for (uint32_t i = 0U; i < MLDSA_N; i++) {
        b->coeffs[i] = MLDSA_ROUNDING_use_hint(a->coeffs[i], (uint32_t) h->coeffs[i], gamma2);
    }
}
