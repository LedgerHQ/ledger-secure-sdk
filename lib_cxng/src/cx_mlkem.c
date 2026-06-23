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
 * @file    cx_mlkem.c
 * @brief   ML-KEM key encapsulation mechanism (FIPS 203).
 *
 * Implements the top-level ML-KEM key generation, encapsulation, and
 * decapsulation routines, including both randomized and derandomized variants.
 */

#include <string.h>

#include "cx_mlkem_internal.h"
#include "cx_mlkem_indcpa.h"
#include "cx_mlkem_util.h"
#include "cx_utils.h"
#include "lcx_rng.h"

cx_err_t MLKEM_crypto_kem_keypair(uint8_t      *pk,
                                  size_t        pk_len,
                                  uint8_t      *sk,
                                  size_t        sk_len,
                                  MLKEM_param_t param)
{
    cx_err_t error                     = CX_INTERNAL_ERROR;
    uint8_t  coins[2 * MLKEM_SYMBYTES] = {0};

    if ((pk == NULL) || (sk == NULL)) {
        error = CX_INVALID_PARAMETER;
        goto end;
    }

    if (param > MLKEM_1024) {
        error = CX_INVALID_PARAMETER_VALUE;
        goto end;
    }

    cx_rng_no_throw(coins, 2 * MLKEM_SYMBYTES);
    error = MLKEM_crypto_kem_keypair_derand(pk, pk_len, sk, sk_len, coins, param);

end:
    explicit_bzero(coins, sizeof(coins));
    return error;
}

cx_err_t MLKEM_crypto_kem_enc(uint8_t       *ct,
                              size_t         ct_len,
                              uint8_t       *ss,
                              size_t         ss_len,
                              const uint8_t *pk,
                              size_t         pk_len,
                              MLKEM_param_t  param)
{
    cx_err_t error                     = CX_INTERNAL_ERROR;
    uint8_t  coins[2 * MLKEM_SYMBYTES] = {0};

    if ((ct == NULL) || (ss == NULL) || (pk == NULL)) {
        error = CX_INVALID_PARAMETER;
        goto end;
    }

    if (param > MLKEM_1024) {
        error = CX_INVALID_PARAMETER_VALUE;
        goto end;
    }

    cx_rng_no_throw(coins, 2 * MLKEM_SYMBYTES);
    error = MLKEM_crypto_kem_enc_derand(ct, ct_len, ss, ss_len, pk, pk_len, coins, param);

end:
    explicit_bzero(coins, sizeof(coins));
    return error;
}

cx_err_t MLKEM_crypto_kem_dec(uint8_t       *ss,
                              size_t         ss_len,
                              const uint8_t *ct,
                              size_t         ct_len,
                              const uint8_t *sk,
                              size_t         sk_len,
                              MLKEM_param_t  param)
{
    cx_err_t error                    = CX_INTERNAL_ERROR;
    uint8_t  buf[2U * MLKEM_SYMBYTES] = {0};
    uint8_t  kr[2U * MLKEM_SYMBYTES]  = {0};
    // Stack optimization: ct2 and tmp share storage: ct2 is used first,
    // then tmp overwrites it
    union {
        uint8_t ct2[MLKEM1024_CIPHERTEXTBYTES];
        uint8_t tmp[MLKEM_SYMBYTES + MLKEM1024_CIPHERTEXTBYTES];
    } u;

    const uint8_t *pk = NULL;
    uint8_t        fail;

    if ((ss == NULL) || (ct == NULL) || (sk == NULL)) {
        error = CX_INVALID_PARAMETER;
        goto end;
    }

    if (param > MLKEM_1024) {
        error = CX_INVALID_PARAMETER_VALUE;
        goto end;
    }

    const MLKEM_param_info_t *p = &MLKEM_PARAM[param];

    if (p == NULL) {
        error = CX_INVALID_PARAMETER;
        goto end;
    }

    if ((sk_len < p->sk_bytes) || (ct_len < p->ct_bytes)) {
        error = CX_INVALID_PARAMETER_SIZE;
        goto end;
    }

    if (ss_len < MLKEM_SYMBYTES) {
        error = CX_INVALID_PARAMETER_SIZE;
        goto end;
    }

    pk = &sk[p->indcpa_sk_bytes];

    // All steps must execute unconditionally for CCA security.
    (void) MLKEM_INDCPA_dec(buf, sizeof(buf), ct, p->ct_bytes, sk, p->sk_bytes, p);

    memcpy(&buf[MLKEM_SYMBYTES], &sk[p->sk_bytes - (2U * MLKEM_SYMBYTES)], MLKEM_SYMBYTES);

    MLKEM_UTIL_hash_g(kr, buf, 2U * MLKEM_SYMBYTES);

    (void) MLKEM_INDCPA_enc(
        u.ct2, sizeof(u.ct2), buf, sizeof(buf), pk, p->pk_bytes, &kr[MLKEM_SYMBYTES], p);
    fail = cx_constant_time_eq(ct, u.ct2, p->ct_bytes);

    // ct2 no longer needed; reuse the union as tmp
    memcpy(u.tmp, &sk[p->sk_bytes - MLKEM_SYMBYTES], MLKEM_SYMBYTES);
    memcpy(&u.tmp[MLKEM_SYMBYTES], ct, p->ct_bytes);
    MLKEM_UTIL_hash_j(ss, u.tmp, MLKEM_SYMBYTES + p->ct_bytes);

    MLKEM_UTIL_ct_cmov(ss, kr, MLKEM_SYMBYTES, (uint8_t) (fail == 0U));

    // Return CX_OK even if MLKEM_INDCPA_dec or MLKEM_INDCPA_enc return an error
    // Decapsulation should be anyway incoherent
    error = CX_OK;

end:
    explicit_bzero(buf, sizeof(buf));
    explicit_bzero(kr, sizeof(kr));
    explicit_bzero(&u, sizeof(u));
    return error;
}
