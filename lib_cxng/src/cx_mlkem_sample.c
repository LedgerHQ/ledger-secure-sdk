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
 * @file    cx_mlkem_sample.c
 * @brief   ML-KEM sampling routines (FIPS 203).
 *
 * Implements rejection sampling (rej_uniform), centered binomial distribution
 * sampling (CBD eta=2 and eta=3), and noise generation for ML-KEM.
 */

#include <string.h>

#include "cx_mlkem_poly.h"
#include "cx_mlkem_util.h"
#include "cx_mlkem_sample.h"

/* ========================================================================
 * Sampling
 * ====================================================================== */

#define XOF_BLOCKBYTES      168U
#define REJ_UNIFORM_NBLOCKS 5U
#define REJ_UNIFORM_BUFLEN  (REJ_UNIFORM_NBLOCKS * XOF_BLOCKBYTES)

/* CBD sampling */
static uint32_t load32_le(const uint8_t *x)
{
    return (uint32_t) x[0] | ((uint32_t) x[1] << 8U) | ((uint32_t) x[2] << 16U)
           | ((uint32_t) x[3] << 24U);
}

static uint32_t load24_le(const uint8_t *x)
{
    return (uint32_t) x[0] | ((uint32_t) x[1] << 8U) | ((uint32_t) x[2] << 16U);
}

void MLKEM_SAMPLE_cbd2(poly *r, const uint8_t *buf)
{
    for (uint32_t i = 0U; i < (MLKEM_N / 8U); i++) {
        uint32_t t = load32_le(&buf[4U * i]);
        uint32_t d = t & 0x55555555U;
        d += (t >> 1U) & 0x55555555U;

        for (uint32_t j = 0U; j < 8U; j++) {
            int16_t a_val           = (int16_t) ((d >> ((4U * j) + 0U)) & 0x3U);
            int16_t b_val           = (int16_t) ((d >> ((4U * j) + 2U)) & 0x3U);
            r->coeffs[(8U * i) + j] = (int16_t) (a_val - b_val);
        }
    }
}

void MLKEM_SAMPLE_cbd3(poly *r, const uint8_t *buf)
{
    for (uint32_t i = 0U; i < (MLKEM_N / 4U); i++) {
        uint32_t t = load24_le(&buf[3U * i]);
        uint32_t d = t & 0x00249249U;
        d += (t >> 1U) & 0x00249249U;
        d += (t >> 2U) & 0x00249249U;

        for (uint32_t j = 0U; j < 4U; j++) {
            int16_t a_val           = (int16_t) ((d >> ((6U * j) + 0U)) & 0x7U);
            int16_t b_val           = (int16_t) ((d >> ((6U * j) + 3U)) & 0x7U);
            r->coeffs[(4U * i) + j] = (int16_t) (a_val - b_val);
        }
    }
}

void MLKEM_SAMPLE_getnoise(poly *r, const uint8_t *seed, uint8_t nonce, uint8_t eta)
{
    uint8_t buf[3 * MLKEM_N / 4] = {0};  // max size for eta=3
    size_t  buflen               = (size_t) eta * MLKEM_N / 4;

    MLKEM_UTIL_prf(buf, buflen, seed, nonce);

    if (eta == 2) {
        MLKEM_SAMPLE_cbd2(r, buf);
    }
    else {  // eta == 3
        MLKEM_SAMPLE_cbd3(r, buf);
    }

    explicit_bzero(buf, sizeof(buf));
}

void MLKEM_SAMPLE_rej_uniform(poly *r, const uint8_t *seed, uint8_t x, uint8_t y)
{
    uint8_t  buf[REJ_UNIFORM_BUFLEN]   = {0};
    uint8_t  buf2[XOF_BLOCKBYTES * 3U] = {0};
    uint32_t ctr                       = 0U;
    uint32_t pos                       = 0U;

    MLKEM_UTIL_xof_squeeze(buf, REJ_UNIFORM_BUFLEN, seed, x, y);

    while ((ctr < MLKEM_N) && ((pos + 3U) <= REJ_UNIFORM_BUFLEN)) {
        int16_t val0 = (int16_t) (((buf[pos] >> 0U) | ((uint16_t) buf[pos + 1U] << 8U)) & 0xFFFU);
        int16_t val1
            = (int16_t) (((buf[pos + 1U] >> 4U) | ((uint16_t) buf[pos + 2U] << 4U)) & 0xFFFU);
        pos += 3U;

        if (val0 < MLKEM_Q) {
            r->coeffs[ctr++] = val0;
        }
        if ((ctr < MLKEM_N) && (val1 < MLKEM_Q)) {
            r->coeffs[ctr++] = val1;
        }
    }

    if (ctr < MLKEM_N) {
        MLKEM_UTIL_xof_squeeze(buf2, sizeof(buf2), seed, x, y);
        pos = 0U;
        while ((ctr < MLKEM_N) && ((pos + 3U) <= sizeof(buf2))) {
            int16_t val0
                = (int16_t) (((buf2[pos] >> 0U) | ((uint16_t) buf2[pos + 1U] << 8U)) & 0xFFFU);
            int16_t val1
                = (int16_t) (((buf2[pos + 1U] >> 4U) | ((uint16_t) buf2[pos + 2U] << 4U)) & 0xFFFU);
            pos += 3U;
            if ((val0 < MLKEM_Q) && (ctr < MLKEM_N)) {
                r->coeffs[ctr++] = val0;
            }
            if ((ctr < MLKEM_N) && (val1 < MLKEM_Q)) {
                r->coeffs[ctr++] = val1;
            }
        }
    }

    explicit_bzero(buf, sizeof(buf));
    explicit_bzero(buf2, sizeof(buf2));
}
