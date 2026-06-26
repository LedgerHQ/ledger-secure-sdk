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
 * @file    cx_mldsa_polyvec.c
 * @brief   ML-DSA polynomial vector operations (FIPS 204).
 */

#include "cx_mldsa_polyvec.h"
#include "cx_mldsa_rounding.h"
#include "cx_mldsa_packing.h"

void MLDSA_POLYVEC_ntt_l(mldsa_polyvecl *v, uint8_t l)
{
    for (uint8_t i = 0U; i < l; i++) {
        MLDSA_POLY_ntt(&v->vec[i]);
    }
}

void MLDSA_POLYVEC_ntt_k(mldsa_polyveck *v, uint8_t k)
{
    for (uint8_t i = 0U; i < k; i++) {
        MLDSA_POLY_ntt(&v->vec[i]);
    }
}

void MLDSA_POLYVEC_invntt_tomont_k(mldsa_polyveck *v, uint8_t k)
{
    for (uint8_t i = 0U; i < k; i++) {
        MLDSA_POLY_invntt_tomont(&v->vec[i]);
    }
}

void MLDSA_POLYVEC_pointwise_acc_montgomery(mldsa_poly           *w,
                                            const mldsa_polyvecl *u,
                                            const mldsa_polyvecl *v,
                                            uint8_t               l)
{
    MLDSA_POLY_pointwise_montgomery(w, &u->vec[0], &v->vec[0], 1);
    for (uint8_t i = 1U; i < l; i++) {
        MLDSA_POLY_pointwise_montgomery(w, &u->vec[i], &v->vec[i], 0);
    }
}

void MLDSA_POLYVEC_add_k(mldsa_polyveck *a, const mldsa_polyveck *b, uint8_t k)
{
    for (uint8_t i = 0U; i < k; i++) {
        MLDSA_POLY_add(&a->vec[i], &b->vec[i]);
    }
}

void MLDSA_POLYVEC_sub_k(mldsa_polyveck *a, const mldsa_polyveck *b, uint8_t k)
{
    for (uint8_t i = 0U; i < k; i++) {
        MLDSA_POLY_sub(&a->vec[i], &b->vec[i]);
    }
}

void MLDSA_POLYVEC_reduce_k(mldsa_polyveck *v, uint8_t k)
{
    for (uint8_t i = 0U; i < k; i++) {
        MLDSA_POLY_reduce(&v->vec[i]);
    }
}

void MLDSA_POLYVEC_caddq_k(mldsa_polyveck *v, uint8_t k)
{
    for (uint8_t i = 0U; i < k; i++) {
        MLDSA_POLY_caddq_all(&v->vec[i]);
    }
}

int MLDSA_POLYVEC_chknorm_l(const mldsa_polyvecl *v, int32_t B, uint8_t l)
{
    for (uint8_t i = 0U; i < l; i++) {
        if (MLDSA_POLY_chknorm(&v->vec[i], B)) {
            return 1;
        }
    }
    return 0;
}

int MLDSA_POLYVEC_chknorm_k(const mldsa_polyveck *v, int32_t B, uint8_t k)
{
    for (uint8_t i = 0U; i < k; i++) {
        if (MLDSA_POLY_chknorm(&v->vec[i], B)) {
            return 1;
        }
    }
    return 0;
}

void MLDSA_POLYVEC_shiftl_k(mldsa_polyveck *v, uint8_t k)
{
    for (uint8_t i = 0U; i < k; i++) {
        MLDSA_POLY_shiftl(&v->vec[i]);
    }
}

void MLDSA_POLYVEC_power2round_k(mldsa_polyveck       *v1,
                                 mldsa_polyveck       *v0,
                                 const mldsa_polyveck *v,
                                 uint8_t               k)
{
    for (uint8_t i = 0U; i < k; i++) {
        MLDSA_ROUNDING_poly_power2round(&v1->vec[i], &v0->vec[i], &v->vec[i]);
    }
}

void MLDSA_POLYVEC_decompose_k(mldsa_polyveck       *v1,
                               mldsa_polyveck       *v0,
                               const mldsa_polyveck *v,
                               int32_t               gamma2,
                               uint8_t               k)
{
    for (uint8_t i = 0U; i < k; i++) {
        MLDSA_ROUNDING_poly_decompose(&v1->vec[i], &v0->vec[i], &v->vec[i], gamma2);
    }
}

uint32_t MLDSA_POLYVEC_make_hint_k(mldsa_polyveck       *h,
                                   const mldsa_polyveck *v0,
                                   const mldsa_polyveck *v1,
                                   int32_t               gamma2,
                                   uint8_t               k)
{
    uint32_t s = 0U;
    for (uint8_t i = 0U; i < k; i++) {
        for (uint32_t j = 0U; j < MLDSA_N; j++) {
            h->vec[i].coeffs[j] = (int32_t) MLDSA_ROUNDING_make_hint(
                v0->vec[i].coeffs[j], v1->vec[i].coeffs[j], gamma2);
            s += (uint32_t) h->vec[i].coeffs[j];
        }
    }
    return s;
}

void MLDSA_POLYVEC_use_hint_k(mldsa_polyveck       *w,
                              const mldsa_polyveck *u,
                              const mldsa_polyveck *h,
                              int32_t               gamma2,
                              uint8_t               k)
{
    for (uint8_t i = 0U; i < k; i++) {
        MLDSA_ROUNDING_poly_use_hint(&w->vec[i], &u->vec[i], &h->vec[i], gamma2);
    }
}

void MLDSA_POLYVEC_pack_w1(uint8_t *r, const mldsa_polyveck *w1, int32_t gamma2, uint8_t k)
{
    uint32_t poly_bytes;
    if (gamma2 == (MLDSA_Q - 1) / 88) {
        poly_bytes = 192U;
    }
    else {
        poly_bytes = 128U;
    }
    for (uint8_t i = 0U; i < k; i++) {
        MLDSA_PACK_polyw1(r + i * poly_bytes, &w1->vec[i], gamma2);
    }
}
