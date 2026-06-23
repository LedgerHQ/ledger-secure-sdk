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
 * @file    cx_mlkem_indcpa.c
 * @brief   ML-KEM IND-CPA key encapsulation (FIPS 203).
 *
 * Implements the IND-CPA-secure key generation, encryption, and decryption
 * routines that form the core building blocks of ML-KEM.
 */

#include <string.h>

#include "cx_mlkem_indcpa.h"
#include "cx_mlkem_poly.h"
#include "cx_mlkem_polyvec.h"
#include "cx_mlkem_polymat.h"
#include "cx_mlkem_util.h"
#include "cx_mlkem_sample.h"

cx_err_t MLKEM_INDCPA_keypair_derand(uint8_t                  *pk,
                                     size_t                    pk_len,
                                     uint8_t                  *sk,
                                     size_t                    sk_len,
                                     const uint8_t             coins[2 * MLKEM_SYMBYTES],
                                     const MLKEM_param_info_t *p)
{
    uint8_t        buf[2U * MLKEM_SYMBYTES]          = {0};
    uint8_t        coins_with_k[MLKEM_SYMBYTES + 1U] = {0};
    const uint8_t *publicseed                        = NULL;
    const uint8_t *noiseseed                         = NULL;
    polyvec        a_row                             = {0};  // one matrix row at a time
    polyvec        e                                 = {0};
    polyvec        pkpv                              = {0};
    polyvec        skpv                              = {0};

    (void) pk_len;
    (void) sk_len;

    memcpy(coins_with_k, coins, MLKEM_SYMBYTES);
    coins_with_k[MLKEM_SYMBYTES] = p->k;
    MLKEM_UTIL_hash_g(buf, coins_with_k, MLKEM_SYMBYTES + 1U);

    publicseed = buf;
    noiseseed  = &buf[MLKEM_SYMBYTES];

    for (uint32_t i = 0U; i < p->k; i++) {
        MLKEM_SAMPLE_getnoise(&skpv.vec[i], noiseseed, (uint8_t) i, p->eta1);
    }

    for (uint32_t i = 0U; i < p->k; i++) {
        MLKEM_SAMPLE_getnoise(&e.vec[i], noiseseed, (uint8_t) (p->k + i), p->eta1);
    }

    MLKEM_POLYVEC_ntt(&skpv, p->k);
    MLKEM_POLYVEC_ntt(&e, p->k);

    // Optimization: Generate matrix A row-by-row to avoid storing full k×k polymat
    for (uint32_t i = 0U; i < p->k; i++) {
        MLKEM_POLYMAT_gen_matrix_row(&a_row, publicseed, 0, (uint8_t) i, p->k);
        MLKEM_POLYVEC_basemul_acc_montgomery(&pkpv.vec[i], &a_row, &skpv, p->k);
    }

    MLKEM_POLYVEC_tomont(&pkpv, p->k);
    MLKEM_POLYVEC_add(&pkpv, &e, p->k);
    MLKEM_POLYVEC_reduce(&pkpv, p->k);
    MLKEM_POLYVEC_reduce(&skpv, p->k);

    MLKEM_POLYVEC_tobytes(sk, &skpv, p->k);
    MLKEM_POLYVEC_tobytes(pk, &pkpv, p->k);
    memcpy(&pk[p->polyvec_bytes], publicseed, MLKEM_SYMBYTES);

    explicit_bzero(buf, sizeof(buf));
    explicit_bzero(coins_with_k, sizeof(coins_with_k));
    explicit_bzero(&a_row, sizeof(a_row));
    explicit_bzero(&e, sizeof(e));
    explicit_bzero(&skpv, sizeof(skpv));

    return CX_OK;
}

cx_err_t MLKEM_INDCPA_enc(uint8_t                  *c,
                          size_t                    c_len,
                          const uint8_t            *m,
                          size_t                    m_len,
                          const uint8_t            *pk,
                          size_t                    pk_len,
                          const uint8_t             coins[2 * MLKEM_SYMBYTES],
                          const MLKEM_param_info_t *p)
{
    uint8_t seed[MLKEM_SYMBYTES] = {0};
    polyvec at_row               = {0};  // one matrix row at a time
    polyvec sp                   = {0};
    polyvec pkpv                 = {0};
    polyvec ep                   = {0};
    polyvec b                    = {0};
    poly    v                    = {0};
    poly    k                    = {0};
    poly    epp                  = {0};

    // TODO: check lengths
    (void) c_len;
    (void) m_len;
    (void) pk_len;

    MLKEM_POLYVEC_frombytes(&pkpv, pk, p->k);
    memcpy(seed, pk + p->polyvec_bytes, MLKEM_SYMBYTES);

    MLKEM_POLY_frommsg(&k, m);

    for (uint32_t i = 0U; i < p->k; i++) {
        MLKEM_SAMPLE_getnoise(&sp.vec[i], coins, (uint8_t) i, p->eta1);
    }

    for (uint32_t i = 0U; i < p->k; i++) {
        MLKEM_SAMPLE_getnoise(&ep.vec[i], coins, (uint8_t) (p->k + i), p->eta2);
    }

    MLKEM_SAMPLE_getnoise(&epp, coins, (uint8_t) (2U * p->k), p->eta2);

    MLKEM_POLYVEC_ntt(&sp, p->k);

    // Optimization: Generate transposed matrix A^T row-by-row
    for (uint32_t i = 0U; i < p->k; i++) {
        MLKEM_POLYMAT_gen_matrix_row(&at_row, seed, 1, (uint8_t) i, p->k);
        MLKEM_POLYVEC_basemul_acc_montgomery(&b.vec[i], &at_row, &sp, p->k);
    }

    MLKEM_POLYVEC_invntt_tomont(&b, p->k);
    MLKEM_POLYVEC_add(&b, &ep, p->k);
    MLKEM_POLYVEC_reduce(&b, p->k);

    MLKEM_POLYVEC_basemul_acc_montgomery(&v, &pkpv, &sp, p->k);
    MLKEM_POLY_invntt_tomont(&v);
    MLKEM_POLY_add(&v, &epp);
    MLKEM_POLY_add(&v, &k);
    MLKEM_POLY_reduce(&v);

    MLKEM_POLYVEC_compress(c, &b, p->k, p->du);

    if (p->dv == 4) {
        MLKEM_POLY_compress_d4(c + p->polyvec_compressed_bytes, &v);
    }
    else {  // dv == 5
        MLKEM_POLY_compress_d5(c + p->polyvec_compressed_bytes, &v);
    }

    explicit_bzero(&at_row, sizeof(at_row));
    explicit_bzero(&sp, sizeof(sp));
    explicit_bzero(&ep, sizeof(ep));
    explicit_bzero(&b, sizeof(b));
    explicit_bzero(&v, sizeof(v));
    explicit_bzero(&epp, sizeof(epp));
    explicit_bzero(&k, sizeof(k));

    return CX_OK;
}

cx_err_t MLKEM_INDCPA_dec(uint8_t                  *m,
                          size_t                    m_len,
                          const uint8_t            *c,
                          size_t                    c_len,
                          const uint8_t            *sk,
                          size_t                    sk_len,
                          const MLKEM_param_info_t *p)
{
    polyvec b    = {0};
    polyvec skpv = {0};
    poly    v    = {0};
    poly    sb   = {0};

    // TODO: check lengths
    (void) c_len;
    (void) m_len;
    (void) sk_len;

    MLKEM_POLYVEC_decompress(&b, c, p->k, p->du);

    if (p->dv == 4) {
        MLKEM_POLY_decompress_d4(&v, c + p->polyvec_compressed_bytes);
    }
    else {  // dv == 5
        MLKEM_POLY_decompress_d5(&v, c + p->polyvec_compressed_bytes);
    }

    MLKEM_POLYVEC_frombytes(&skpv, sk, p->k);

    MLKEM_POLYVEC_ntt(&b, p->k);
    MLKEM_POLYVEC_basemul_acc_montgomery(&sb, &skpv, &b, p->k);
    MLKEM_POLY_invntt_tomont(&sb);
    MLKEM_POLY_sub(&v, &sb);
    MLKEM_POLY_reduce(&v);
    MLKEM_POLY_tomsg(m, &v);

    explicit_bzero(&skpv, sizeof(skpv));
    explicit_bzero(&sb, sizeof(sb));
    explicit_bzero(&v, sizeof(v));

    return CX_OK;
}
