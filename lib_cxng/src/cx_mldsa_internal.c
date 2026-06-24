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
 * @file    cx_mldsa_internal.c
 * @brief   ML-DSA internal functions for testing (FIPS 204).
 *
 * These functions expose deterministic key generation, signing, and
 * verification entry points intended only for conformance testing.
 * They should not be called directly in production code.
 */

#include <string.h>
#include "lcx_mldsa.h"
#include "cx_mldsa_internal.h"
#include "cx_mldsa_poly.h"
#include "cx_mldsa_polyvec.h"
#include "cx_mldsa_polymat.h"
#include "cx_mldsa_sample.h"
#include "cx_mldsa_packing.h"
#include "cx_mldsa_rounding.h"
#include "cx_mldsa_util.h"

/*---------------------------------------------------------------------------
 * Internal: KeyGen_internal (FIPS 204, Algorithm 6)
 *---------------------------------------------------------------------------*/
/*
 * Low-RAM key generation.
 *
 * The matrix A and the secret vector s1 are never stored as full vectors.
 * t = A*s1 + s2 is computed one output row at a time; within a row the
 * inner product is accumulated element-by-element, re-expanding A[i][j]
 * and s1[j] on the fly.  Each produced polynomial (s1, s2, t0, t1) is
 * packed straight into the pk/sk output buffers, so no unpacked polyvec
 * is ever held in memory.
 */
cx_err_t MLDSA_internal_keygen(uint8_t      *pk,
                               size_t        pk_len,
                               uint8_t      *sk,
                               size_t        sk_len,
                               const uint8_t seed[MLDSA_SEEDBYTES],
                               MLDSA_param_t param)
{
    cx_err_t                  error                                          = CX_INTERNAL_ERROR;
    const MLDSA_param_info_t *p                                              = NULL;
    uint8_t                   seedbuf[2U * MLDSA_SEEDBYTES + MLDSA_CRHBYTES] = {0};
    uint8_t                   buf[MLDSA_SEEDBYTES + 2U]                      = {0};
    uint8_t                   tr[MLDSA_TRBYTES]                              = {0};
    mldsa_poly                tA                                             = {0};
    mldsa_poly                tB                                             = {0};
    mldsa_poly                tC                                             = {0};

    if ((pk == NULL) || (sk == NULL) || (seed == NULL)) {
        error = CX_INVALID_PARAMETER;
        goto cleanup;
    }

    if (param >= MLDSA_NUM_PARAM_SETS) {
        error = CX_INVALID_PARAMETER_VALUE;
        goto cleanup;
    }

    p = &MLDSA_PARAM[param];

    if ((pk_len < p->pk_bytes) || (sk_len < p->sk_bytes)) {
        error = CX_INVALID_PARAMETER_SIZE;
        goto cleanup;
    }

    // Expand seed to (rho, rhoprime, K) using SHAKE256
    memcpy(buf, seed, MLDSA_SEEDBYTES);
    buf[MLDSA_SEEDBYTES]      = p->k;
    buf[MLDSA_SEEDBYTES + 1U] = p->l;
    MLDSA_UTIL_shake256(seedbuf, sizeof(seedbuf), buf, MLDSA_SEEDBYTES + 2U);

    const uint8_t *rho      = seedbuf;
    const uint8_t *rhoprime = &seedbuf[MLDSA_SEEDBYTES];
    const uint8_t *K        = &seedbuf[MLDSA_SEEDBYTES + MLDSA_CRHBYTES];

    // Secret-key section offsets.
    const size_t s1_off = 2U * MLDSA_SEEDBYTES + MLDSA_TRBYTES;
    const size_t s2_off = s1_off + (size_t) p->l * p->polyeta_packed_bytes;
    const size_t t0_off = s1_off + (size_t) (p->l + p->k) * p->polyeta_packed_bytes;

    memcpy(sk, rho, MLDSA_SEEDBYTES);
    memcpy(&sk[MLDSA_SEEDBYTES], K, MLDSA_SEEDBYTES);
    memcpy(pk, rho, MLDSA_SEEDBYTES);

    // Optimization: compute t = A*s1 + s2 one row at a time, holding only 3 polynomials.
    for (uint8_t i = 0U; i < p->k; i++) {
        for (uint8_t j = 0U; j < p->l; j++) {
            // Expand s1[j] on the fly; pack it once (first output row only).
            MLDSA_SAMPLE_eta(&tC, rhoprime, j, p->eta);
            if (i == 0U) {
                MLDSA_PACK_polyeta(&sk[s1_off + (size_t) j * p->polyeta_packed_bytes], &tC, p->eta);
            }
            MLDSA_POLY_ntt(&tC);

            // Expand A[i][j] on the fly and accumulate into the row.
            uint16_t nonce = ((uint16_t) i << 8U) | (uint16_t) j;
            MLDSA_SAMPLE_uniform(&tB, rho, nonce);
            MLDSA_POLY_pointwise_montgomery(&tA, &tB, &tC, (j == 0U) ? 1 : 0);
        }

        MLDSA_POLY_reduce(&tA);
        MLDSA_POLY_invntt_tomont(&tA);

        // Sample and pack the error term s2[i], then add it to the row.
        MLDSA_SAMPLE_eta(&tB, rhoprime, (uint16_t) (p->l + i), p->eta);
        MLDSA_PACK_polyeta(&sk[s2_off + (size_t) i * p->polyeta_packed_bytes], &tB, p->eta);
        MLDSA_POLY_add(&tA, &tB);
        MLDSA_POLY_caddq_all(&tA);

        // Split into (t1, t0)
        MLDSA_ROUNDING_poly_power2round(&tC, &tB, &tA);
        MLDSA_PACK_polyt0(&sk[t0_off + (size_t) i * MLDSA_POLYT0_PACKEDBYTES], &tB);
        MLDSA_PACK_polyt1(&pk[MLDSA_SEEDBYTES + (size_t) i * MLDSA_POLYT1_PACKEDBYTES], &tC);
    }

    // Compute tr = H(pk) = SHAKE256(pk, 64) and store it in the secret key.
    MLDSA_UTIL_shake256(tr, MLDSA_TRBYTES, pk, p->pk_bytes);
    memcpy(&sk[2U * MLDSA_SEEDBYTES], tr, MLDSA_TRBYTES);

    error = CX_OK;

cleanup:
    // Zeroize sensitive intermediates
    explicit_bzero(seedbuf, sizeof(seedbuf));
    explicit_bzero(buf, sizeof(buf));
    explicit_bzero(&tA, sizeof(tA));
    explicit_bzero(&tB, sizeof(tB));
    explicit_bzero(&tC, sizeof(tC));
    explicit_bzero(tr, sizeof(tr));

    return error;
}

/*---------------------------------------------------------------------------
 * Internal Sign (FIPS 204, Algorithms 2 & 7)
 *---------------------------------------------------------------------------*/
cx_err_t MLDSA_internal_sign(uint8_t       *sig,
                             size_t         sig_len,
                             size_t        *sig_actual_len,
                             const uint8_t *mu,
                             uint8_t       *rnd,
                             size_t         rnd_len,
                             const uint8_t *sk,
                             size_t         sk_len,
                             MLDSA_param_t  param)
{
    if (mu == NULL) {
        return CX_INVALID_PARAMETER;
    }

    return MLDSA_internal_sign_core(
        sig, sig_len, sig_actual_len, NULL, mu, rnd, rnd_len, sk, sk_len, param);
}

/*---------------------------------------------------------------------------
 * Internal Verify (FIPS 204, Algorithms 3 & 8)
 *---------------------------------------------------------------------------*/
cx_err_t MLDSA_internal_verify(const uint8_t *sig,
                               size_t         sig_len,
                               const uint8_t *mu,
                               const uint8_t *pk,
                               size_t         pk_len,
                               MLDSA_param_t  param)
{
    if ((sig == NULL) || (pk == NULL) || (mu == NULL)) {
        return CX_INVALID_PARAMETER;
    }

    return MLDSA_internal_verify_core(sig, sig_len, NULL, mu, pk, pk_len, param);
}
