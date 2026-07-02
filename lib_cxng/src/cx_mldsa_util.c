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
 * @file    cx_mldsa_util.c
 * @brief   ML-DSA utility functions (FIPS 204).
 */

#include <string.h>
#include "lcx_mldsa.h"
#include "cx_mldsa_util.h"
#include "lcx_sha3.h"
#include "lcx_hash.h"

void MLDSA_UTIL_shake256(uint8_t *out, size_t outlen, const uint8_t *in, size_t inlen)
{
    cx_shake256_hash(in, inlen, out, outlen);
}

void MLDSA_UTIL_shake128(uint8_t *out, size_t outlen, const uint8_t *in, size_t inlen)
{
    cx_shake128_hash(in, inlen, out, outlen);
}

void MLDSA_UTIL_shake256_two(uint8_t       *out,
                             size_t         outlen,
                             const uint8_t *in1,
                             size_t         in1len,
                             const uint8_t *in2,
                             size_t         in2len)
{
    uint8_t buf[256] = {0};
    size_t  total    = in1len + in2len;

    if (total <= sizeof(buf)) {
        (void) memcpy(buf, in1, in1len);
        (void) memcpy(buf + in1len, in2, in2len);
        cx_shake256_hash(buf, total, out, outlen);
        explicit_bzero(buf, total);
    }
    else {
        cx_sha3_t ctx = {0};
        cx_shake256_init_no_throw(&ctx, outlen * 8U);
        cx_hash_no_throw((cx_hash_t *) &ctx, 0, in1, in1len, NULL, 0);
        cx_hash_no_throw((cx_hash_t *) &ctx, CX_LAST, in2, in2len, out, outlen);
    }
}

void MLDSA_UTIL_shake256_three(uint8_t       *out,
                               size_t         outlen,
                               const uint8_t *in1,
                               size_t         in1len,
                               const uint8_t *in2,
                               size_t         in2len,
                               const uint8_t *in3,
                               size_t         in3len)
{
    cx_sha3_t ctx = {0};
    cx_shake256_init_no_throw(&ctx, outlen * 8U);
    cx_hash_no_throw((cx_hash_t *) &ctx, 0, in1, in1len, NULL, 0);
    cx_hash_no_throw((cx_hash_t *) &ctx, 0, in2, in2len, NULL, 0);
    cx_hash_no_throw((cx_hash_t *) &ctx, CX_LAST, in3, in3len, out, outlen);
}

void MLDSA_UTIL_shake128_seed_nonce(uint8_t       *out,
                                    size_t         outlen,
                                    const uint8_t *seed,
                                    size_t         seedlen,
                                    uint16_t       nonce)
{
    uint8_t buf[MLDSA_SEEDBYTES + 2U] = {0};
    if (seedlen > MLDSA_SEEDBYTES) {
        seedlen = MLDSA_SEEDBYTES;
    }
    memcpy(buf, seed, seedlen);
    buf[seedlen]      = (uint8_t) (nonce & 0xFFU);
    buf[seedlen + 1U] = (uint8_t) (nonce >> 8U);
    cx_shake128_hash(buf, seedlen + 2U, out, outlen);
    explicit_bzero(buf, sizeof(buf));
}

void MLDSA_UTIL_shake256_seed_nonce(uint8_t       *out,
                                    size_t         outlen,
                                    const uint8_t *seed,
                                    size_t         seedlen,
                                    uint16_t       nonce)
{
    uint8_t buf[MLDSA_CRHBYTES + 2U] = {0};
    if (seedlen > MLDSA_CRHBYTES) {
        seedlen = MLDSA_CRHBYTES;
    }
    memcpy(buf, seed, seedlen);
    buf[seedlen]      = (uint8_t) (nonce & 0xFFU);
    buf[seedlen + 1U] = (uint8_t) (nonce >> 8U);
    cx_shake256_hash(buf, seedlen + 2U, out, outlen);
    explicit_bzero(buf, sizeof(buf));
}
