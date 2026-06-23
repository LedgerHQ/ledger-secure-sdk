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
    mldsa_polyvecl            s1                                             = {0};
    mldsa_polyveck            t                                              = {0};
    mldsa_poly                tmp_poly                                       = {0};
    mldsa_poly                t0_row                                         = {0};

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
    (void) memcpy(buf, seed, MLDSA_SEEDBYTES);
    buf[MLDSA_SEEDBYTES]      = p->k;
    buf[MLDSA_SEEDBYTES + 1U] = p->l;
    MLDSA_UTIL_shake256(seedbuf, sizeof(seedbuf), buf, MLDSA_SEEDBYTES + 2U);

    const uint8_t *rho      = seedbuf;
    const uint8_t *rhoprime = &seedbuf[MLDSA_SEEDBYTES];
    const uint8_t *K        = &seedbuf[MLDSA_SEEDBYTES + MLDSA_CRHBYTES];

    // Expand matrix A on the fly and compute t = A*NTT(s1)
    for (uint8_t i = 0U; i < p->l; i++) {
        MLDSA_SAMPLE_eta(&s1.vec[i], rhoprime, i, p->eta);
    }

    MLDSA_POLYVEC_ntt_l(&s1, p->l);
    MLDSA_POLYMAT_expand_and_multiply(&t, rho, &s1, p);

    // Re-sample s1 in normal form (deterministic, same rhoprime + nonces)
    for (uint8_t i = 0U; i < p->l; i++) {
        MLDSA_SAMPLE_eta(&s1.vec[i], rhoprime, i, p->eta);
    }

    MLDSA_POLYVEC_reduce_k(&t, p->k);
    MLDSA_POLYVEC_invntt_tomont_k(&t, p->k);

    // Streamed s2: sample row, add to t row, and pack row directly into sk.
    for (uint8_t i = 0U; i < p->k; i++) {
        MLDSA_SAMPLE_eta(&tmp_poly, rhoprime, (uint16_t) (p->l + i), p->eta);
        MLDSA_POLY_add(&t.vec[i], &tmp_poly);
        MLDSA_POLY_caddq_all(&t.vec[i]);
        MLDSA_PACK_polyeta(
            &sk[(2U * MLDSA_SEEDBYTES + MLDSA_TRBYTES) + (size_t) p->l * p->polyeta_packed_bytes
                + (size_t) i * p->polyeta_packed_bytes],
            &tmp_poly,
            p->eta);
    }

    // Streamed power2round: keep t1 in t[i], pack t0 row directly into sk.
    for (uint8_t i = 0U; i < p->k; i++) {
        MLDSA_ROUNDING_poly_power2round(&t.vec[i], &t0_row, &t.vec[i]);
        MLDSA_PACK_polyt0(&sk[(2U * MLDSA_SEEDBYTES + MLDSA_TRBYTES)
                              + (size_t) (p->l + p->k) * p->polyeta_packed_bytes
                              + (size_t) i * MLDSA_POLYT0_PACKEDBYTES],
                          &t0_row);
    }

    // Pack public key: pk = (rho, t1)
    MLDSA_PACK_pk(pk, rho, t.vec, p);

    // Compute tr = H(pk) = SHAKE256(pk, 64)
    MLDSA_UTIL_shake256(tr, MLDSA_TRBYTES, pk, p->pk_bytes);

    // Fill secret key header and streamed s1 section.
    (void) memcpy(sk, rho, MLDSA_SEEDBYTES);
    (void) memcpy(&sk[MLDSA_SEEDBYTES], K, MLDSA_SEEDBYTES);
    (void) memcpy(&sk[2U * MLDSA_SEEDBYTES], tr, MLDSA_TRBYTES);

    for (uint8_t i = 0U; i < p->l; i++) {
        MLDSA_SAMPLE_eta(&s1.vec[i], rhoprime, i, p->eta);
        MLDSA_PACK_polyeta(
            &sk[(2U * MLDSA_SEEDBYTES + MLDSA_TRBYTES) + (size_t) i * p->polyeta_packed_bytes],
            &s1.vec[i],
            p->eta);
    }

    error = CX_OK;

cleanup:
    // Zeroize sensitive intermediates
    explicit_bzero(seedbuf, sizeof(seedbuf));
    explicit_bzero(buf, sizeof(buf));
    explicit_bzero(&s1, sizeof(s1));
    explicit_bzero(&tmp_poly, sizeof(tmp_poly));
    explicit_bzero(&t0_row, sizeof(t0_row));
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
