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
 * @file    cx_mlkem_internal.c
 * @brief   Internal ML-KEM deterministic routines (not part of the public API).
 *
 * Contains the derandomized key-generation and encapsulation functions
 * that are used internally and exposed only for testing.
 */

#include <string.h>

#include "cx_mlkem_internal.h"
#include "cx_mlkem_indcpa.h"
#include "cx_mlkem_util.h"
#include "cx_utils.h"

cx_err_t MLKEM_crypto_kem_keypair_derand(uint8_t      *pk,
                                         size_t        pk_len,
                                         uint8_t      *sk,
                                         size_t        sk_len,
                                         const uint8_t coins[2 * MLKEM_SYMBYTES],
                                         MLKEM_param_t param)
{
    cx_err_t error = CX_INTERNAL_ERROR;

    if ((pk == NULL) || (sk == NULL)) {
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

    if ((pk_len < p->pk_bytes) || (sk_len < p->sk_bytes)) {
        error = CX_INVALID_PARAMETER_SIZE;
        goto end;
    }

    if ((p->sk_bytes < (2U * MLKEM_SYMBYTES)) || (p->indcpa_sk_bytes + p->pk_bytes > sk_len)) {
        error = CX_INVALID_PARAMETER_SIZE;
        goto end;
    }

    CX_CHECK(MLKEM_INDCPA_keypair_derand(pk, p->pk_bytes, sk, p->sk_bytes, coins, p));

    memcpy(&sk[p->indcpa_sk_bytes], pk, p->pk_bytes);
    MLKEM_UTIL_hash_h(&sk[p->sk_bytes - (2U * MLKEM_SYMBYTES)], pk, p->pk_bytes);
    memcpy(&sk[p->sk_bytes - MLKEM_SYMBYTES], &coins[MLKEM_SYMBYTES], MLKEM_SYMBYTES);

end:
    return error;
}

cx_err_t MLKEM_crypto_kem_enc_derand(uint8_t       *ct,
                                     size_t         ct_len,
                                     uint8_t       *ss,
                                     size_t         ss_len,
                                     const uint8_t *pk,
                                     size_t         pk_len,
                                     const uint8_t  coins[2 * MLKEM_SYMBYTES],
                                     MLKEM_param_t  param)
{
    cx_err_t error                    = CX_INTERNAL_ERROR;
    uint8_t  buf[2U * MLKEM_SYMBYTES] = {0};
    uint8_t  kr[2U * MLKEM_SYMBYTES]  = {0};

    if ((ct == NULL) || (ss == NULL) || (pk == NULL)) {
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

    if ((pk_len < p->pk_bytes) || (ct_len < p->ct_bytes)) {
        error = CX_INVALID_PARAMETER_SIZE;
        goto end;
    }

    // FIPS 203 §7.2: validate encapsulation key modulus
    CX_CHECK(MLKEM_UTIL_check_ek(pk, p));

    memcpy(buf, coins, MLKEM_SYMBYTES);
    MLKEM_UTIL_hash_h(&buf[MLKEM_SYMBYTES], pk, p->pk_bytes);
    MLKEM_UTIL_hash_g(kr, buf, 2U * MLKEM_SYMBYTES);

    CX_CHECK(MLKEM_INDCPA_enc(
        ct, p->ct_bytes, buf, sizeof(buf), pk, p->pk_bytes, &kr[MLKEM_SYMBYTES], p));

    if (ss_len < MLKEM_SYMBYTES) {
        error = CX_INVALID_PARAMETER_SIZE;
        goto end;
    }

    memcpy(ss, kr, MLKEM_SYMBYTES);

end:
    explicit_bzero(buf, sizeof(buf));
    explicit_bzero(kr, sizeof(kr));
    return error;
}
