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
 * @file    cx_mldsa_sample.c
 * @brief   ML-DSA sampling operations (FIPS 204).
 *
 * Implements rejection sampling for uniform, eta, gamma1, and challenge
 * polynomial generation.
 */

#include <string.h>
#include "cx_mldsa_sample.h"
#include "cx_mldsa_util.h"

// SHAKE128 block size for uniform sampling.
#define MLDSA_XOF_BLOCKBYTES 168U

// Rejection sampling buffer for uniform sampling.
// 5 SHAKE128 blocks = 840 bytes = 280 candidates (3 bytes each).
// Acceptance rate is q/2^23 ≈ 0.9994, so 280 candidates are vastly
// more than the 256 needed.
#define MLDSA_REJ_UNIFORM_NBLOCKS 5U
#define MLDSA_REJ_UNIFORM_BUFLEN  (MLDSA_REJ_UNIFORM_NBLOCKS * MLDSA_XOF_BLOCKBYTES)

void MLDSA_SAMPLE_uniform(mldsa_poly *a, const uint8_t seed[MLDSA_SEEDBYTES], uint16_t nonce)
{
    uint8_t  buf[MLDSA_REJ_UNIFORM_BUFLEN];
    uint32_t ctr = 0U;

    MLDSA_UTIL_shake128_seed_nonce(buf, sizeof(buf), seed, MLDSA_SEEDBYTES, nonce);

    uint32_t pos = 0U;
    while ((ctr < MLDSA_N) && ((pos + 3U) <= sizeof(buf))) {
        uint32_t t;
        t = (uint32_t) buf[pos++];
        t |= (uint32_t) buf[pos++] << 8U;
        t |= (uint32_t) buf[pos++] << 16U;
        t &= 0x7FFFFFU;

        if (t < (uint32_t) MLDSA_Q) {
            a->coeffs[ctr] = (int32_t) t;
            ctr++;
        }
    }

    // Fill remaining coefficients with 0 if exhausted (unlikely)
    while (ctr < MLDSA_N) {
        a->coeffs[ctr++] = 0;
    }

    explicit_bzero(buf, sizeof(buf));
}

void MLDSA_SAMPLE_eta(mldsa_poly   *a,
                      const uint8_t seed[MLDSA_CRHBYTES],
                      uint16_t      nonce,
                      uint8_t       eta)
{
    // SHAKE256 rate is 136 bytes.
    // For eta=2: P(accept) = 15/16 per nibble. 2 blocks = 272 bytes = 544 nibbles,
    //   expected ~510 accepted. Very safe for 256 needed.
    // For eta=4: P(accept) = 9/16 per nibble. 4 blocks = 544 bytes = 1088 nibbles,
    //   expected ~612 accepted. Very safe for 256 needed.
    uint8_t buf[136U * 4U];
    size_t  buflen;

    if (eta == 2U) {
        buflen = 136U * 2U;
    }
    else {
        buflen = 136U * 4U;
    }

    MLDSA_UTIL_shake256_seed_nonce(buf, buflen, seed, MLDSA_CRHBYTES, nonce);

    uint32_t ctr = 0U;
    uint32_t pos = 0U;

    if (eta == 2U) {
        while ((ctr < MLDSA_N) && (pos < buflen)) {
            uint8_t t0 = buf[pos] & 0x0FU;
            uint8_t t1 = buf[pos] >> 4U;
            pos++;

            if (t0 < 15U) {
                // Map [0,14] to [-2,2] via: eta - (t mod (2*eta+1)) = 2 - (t mod 5)
                a->coeffs[ctr] = (int32_t) (2U - (t0 % 5U));
                ctr++;
            }
            if ((t1 < 15U) && (ctr < MLDSA_N)) {
                a->coeffs[ctr] = (int32_t) (2U - (t1 % 5U));
                ctr++;
            }
        }
    }
    else {
        // eta == 4
        while ((ctr < MLDSA_N) && (pos < buflen)) {
            uint8_t t0 = buf[pos] & 0x0FU;
            uint8_t t1 = buf[pos] >> 4U;
            pos++;

            if (t0 < 9U) {
                a->coeffs[ctr] = (int32_t) (4 - t0);
                ctr++;
            }
            if ((t1 < 9U) && (ctr < MLDSA_N)) {
                a->coeffs[ctr] = (int32_t) (4 - t1);
                ctr++;
            }
        }
    }

    explicit_bzero(buf, sizeof(buf));
}

void MLDSA_SAMPLE_gamma1(mldsa_poly   *a,
                         const uint8_t seed[MLDSA_CRHBYTES],
                         uint16_t      nonce,
                         int32_t       gamma1)
{
    // gamma1 = 2^17 => 18 bits/coeff => 576 bytes.
    // gamma1 = 2^19 => 20 bits/coeff => 640 bytes.
    uint8_t buf[640U];
    size_t  buflen;

    if (gamma1 == (1 << 17)) {
        buflen = 576U;
    }
    else {
        buflen = 640U;
    }

    MLDSA_UTIL_shake256_seed_nonce(buf, buflen, seed, MLDSA_CRHBYTES, nonce);

    if (gamma1 == (1 << 17)) {
        // 18 bits per coefficient
        for (uint32_t i = 0U; i < MLDSA_N / 4U; i++) {
            uint32_t t0, t1, t2, t3;
            t0 = (uint32_t) buf[9U * i + 0U];
            t0 |= (uint32_t) buf[9U * i + 1U] << 8U;
            t0 |= (uint32_t) buf[9U * i + 2U] << 16U;
            t0 &= 0x3FFFFU;

            t1 = (uint32_t) buf[9U * i + 2U] >> 2U;
            t1 |= (uint32_t) buf[9U * i + 3U] << 6U;
            t1 |= (uint32_t) buf[9U * i + 4U] << 14U;
            t1 &= 0x3FFFFU;

            t2 = (uint32_t) buf[9U * i + 4U] >> 4U;
            t2 |= (uint32_t) buf[9U * i + 5U] << 4U;
            t2 |= (uint32_t) buf[9U * i + 6U] << 12U;
            t2 &= 0x3FFFFU;

            t3 = (uint32_t) buf[9U * i + 6U] >> 6U;
            t3 |= (uint32_t) buf[9U * i + 7U] << 2U;
            t3 |= (uint32_t) buf[9U * i + 8U] << 10U;
            t3 &= 0x3FFFFU;

            a->coeffs[4U * i + 0U] = gamma1 - (int32_t) t0;
            a->coeffs[4U * i + 1U] = gamma1 - (int32_t) t1;
            a->coeffs[4U * i + 2U] = gamma1 - (int32_t) t2;
            a->coeffs[4U * i + 3U] = gamma1 - (int32_t) t3;
        }
    }
    else {
        // 20 bits per coefficient
        for (uint32_t i = 0U; i < MLDSA_N / 2U; i++) {
            uint32_t t0, t1;
            t0 = (uint32_t) buf[5U * i + 0U];
            t0 |= (uint32_t) buf[5U * i + 1U] << 8U;
            t0 |= (uint32_t) buf[5U * i + 2U] << 16U;
            t0 &= 0xFFFFFU;

            t1 = (uint32_t) buf[5U * i + 2U] >> 4U;
            t1 |= (uint32_t) buf[5U * i + 3U] << 4U;
            t1 |= (uint32_t) buf[5U * i + 4U] << 12U;
            t1 &= 0xFFFFFU;

            a->coeffs[2U * i + 0U] = gamma1 - (int32_t) t0;
            a->coeffs[2U * i + 1U] = gamma1 - (int32_t) t1;
        }
    }

    explicit_bzero(buf, sizeof(buf));
}

void MLDSA_SAMPLE_challenge(mldsa_poly *c, const uint8_t *seed, size_t seedlen, uint8_t tau)
{
    // 2 SHAKE256 blocks = 272 bytes. Challenge sampling needs 8 sign bytes
    // plus at most ~128 bytes for tau=60. 272 bytes is more than sufficient.
    uint8_t  buf[136U * 2U];
    uint64_t signs;

    MLDSA_UTIL_shake256(buf, sizeof(buf), seed, seedlen);

    // First 8 bytes give the sign bits
    signs = 0U;
    for (uint32_t i = 0U; i < 8U; i++) {
        signs |= (uint64_t) buf[i] << (8U * i);
    }

    // Zero out all coefficients
    for (uint32_t i = 0U; i < MLDSA_N; i++) {
        c->coeffs[i] = 0;
    }

    uint32_t pos = 8U;
    for (uint32_t i = MLDSA_N - (uint32_t) tau; i < MLDSA_N; i++) {
        uint32_t j;
        // Sample j uniformly from [0, i]
        do {
            if (pos >= sizeof(buf)) {
                // Buffer exhausted (unlikely with 272 bytes); default to no-op swap
                j = i;
                break;
            }
            j = (uint32_t) buf[pos++];
        } while (j > i);

        c->coeffs[i] = c->coeffs[j];
        c->coeffs[j] = 1 - 2 * (int32_t) (signs & 1U);
        signs >>= 1U;
    }

    explicit_bzero(buf, sizeof(buf));
}
