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
 * @file    cx_mlkem_util.c
 * @brief   ML-KEM utility functions (FIPS 203).
 *
 * Implements the hash functions H (SHA3-256) and G (SHA3-512), the PRF
 * (SHAKE256), the XOF (SHAKE128) used throughout the ML-KEM implementation.
 */

#include <string.h>

#include "lcx_mlkem.h"
#include "cx_mlkem_util.h"
#include "lcx_sha3.h"

void MLKEM_UTIL_hash_h(uint8_t out[CX_SHA3_256_SIZE], const uint8_t *in, size_t in_len)
{
    cx_sha3_256_hash(in, in_len, out);
}

void MLKEM_UTIL_hash_g(uint8_t *out, const uint8_t *in, size_t in_len)
{
    cx_sha3_512_hash(in, in_len, out);
}

void MLKEM_UTIL_hash_j(uint8_t *out, const uint8_t *in, size_t inlen)
{
    cx_shake256_hash(in, inlen, out, MLKEM_SYMBYTES);
}

void MLKEM_UTIL_prf(uint8_t *out, size_t outlen, const uint8_t *key, uint8_t nonce)
{
    uint8_t extkey[MLKEM_SYMBYTES + 1U] = {0};
    (void) memcpy(extkey, key, MLKEM_SYMBYTES);
    extkey[MLKEM_SYMBYTES] = nonce;
    cx_shake256_hash(extkey, MLKEM_SYMBYTES + 1U, out, outlen);
    explicit_bzero(extkey, sizeof(extkey));
}

void MLKEM_UTIL_xof_squeeze(uint8_t *out, size_t outlen, const uint8_t *seed, uint8_t x, uint8_t y)
{
    uint8_t extseed[MLKEM_SYMBYTES + 2U] = {0};
    (void) memcpy(extseed, seed, MLKEM_SYMBYTES);
    extseed[MLKEM_SYMBYTES]      = x;
    extseed[MLKEM_SYMBYTES + 1U] = y;
    cx_shake128_hash(extseed, MLKEM_SYMBYTES + 2U, out, outlen);
}

void MLKEM_UTIL_ct_cmov(uint8_t *dst, const uint8_t *src, size_t len, uint8_t b)
{
    uint8_t mask = (uint8_t) (-(int8_t) (b != 0U));
    for (size_t i = 0U; i < len; i++) {
        dst[i] ^= mask & (dst[i] ^ src[i]);
    }
}

cx_err_t MLKEM_UTIL_check_ek(const uint8_t *ek, const MLKEM_param_info_t *p)
{
    if ((p == NULL) || (ek == NULL)) {
        return CX_INVALID_PARAMETER;
    }
    for (uint32_t i = 0U; i < p->k; i++) {
        const uint8_t *a = &ek[i * MLKEM_POLYBYTES];
        for (uint32_t j = 0U; j < (MLKEM_N / 2U); j++) {
            uint16_t c0 = (uint16_t) (a[3U * j] | (((uint16_t) a[(3U * j) + 1U] << 8U) & 0xFFFU));
            uint16_t c1
                = (uint16_t) ((a[(3U * j) + 1U] >> 4U) | ((uint16_t) a[(3U * j) + 2U] << 4U));
            if ((c0 >= MLKEM_Q) || (c1 >= MLKEM_Q)) {
                return CX_INVALID_PARAMETER;
            }
        }
    }
    return CX_OK;
}
