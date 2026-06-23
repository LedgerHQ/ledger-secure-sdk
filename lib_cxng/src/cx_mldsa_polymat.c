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

/**
 * @file    cx_mldsa_polymat.c
 * @brief   ML-DSA matrix operations (FIPS 204).
 */

#include "cx_mldsa_polymat.h"
#include "cx_mldsa_sample.h"

void MLDSA_POLYMAT_expand(mldsa_polyvecl           *mat,
                          const uint8_t             rho[MLDSA_SEEDBYTES],
                          const MLDSA_param_info_t *p)
{
    for (uint8_t i = 0U; i < p->k; i++) {
        for (uint8_t j = 0U; j < p->l; j++) {
            uint16_t nonce = ((uint16_t) i << 8U) | (uint16_t) j;
            MLDSA_SAMPLE_uniform(&mat[i].vec[j], rho, nonce);
        }
    }
}

void MLDSA_POLYMAT_pointwise_montgomery(mldsa_polyveck           *t,
                                        const mldsa_polyvecl     *mat,
                                        const mldsa_polyvecl     *s,
                                        const MLDSA_param_info_t *p)
{
    for (uint8_t i = 0U; i < p->k; i++) {
        MLDSA_POLYVEC_pointwise_acc_montgomery(&t->vec[i], &mat[i], s, p->l);
    }
}

void MLDSA_POLYMAT_expand_and_multiply(mldsa_polyveck           *t,
                                       const uint8_t             rho[MLDSA_SEEDBYTES],
                                       const mldsa_polyvecl     *s,
                                       const MLDSA_param_info_t *p)
{
    mldsa_poly aij;
    for (uint8_t i = 0U; i < p->k; i++) {
        for (uint8_t j = 0U; j < p->l; j++) {
            uint16_t nonce = ((uint16_t) i << 8U) | (uint16_t) j;
            MLDSA_SAMPLE_uniform(&aij, rho, nonce);
            MLDSA_POLY_pointwise_montgomery(&t->vec[i], &aij, &s->vec[j], (j == 0U) ? 1 : 0);
        }
    }
}
