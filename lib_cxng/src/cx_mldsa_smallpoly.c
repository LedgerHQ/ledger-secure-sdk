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
 * @file    cx_mldsa_smallpoly.c
 * @brief   ML-DSA small NTT mod 3329 for low-RAM optimization.
 *
 * Implements 16-bit NTT / inverse NTT / basemul using the Kyber prime 3329.
 * This enables multiplying the challenge polynomial c (TAU nonzero +/-1 coefficients)
 * by secret polynomials s1/s2 (coefficients in [-eta, eta]) using only 512 bytes
 * per polynomial instead of 1024 bytes.
 */

#ifdef HAVE_MLDSA_OPTIMIZATION

#include "cx_mldsa_smallpoly.h"
#include <string.h>

/*********************
 *  STATIC DATA
 *********************/

// clang-format off
/**
 * Precomputed zetas for NTT mod 3329 (same as Kyber).
 * mont = 2^16 mod 3329 = -1044.
 */
static const int16_t mldsa_small_zetas[128] = {
    -1044, -758,  -359,  -1517, 1493,  1422,  287,   202,
    -171,  622,   1577,  182,   962,   -1202, -1474, 1468,
    573,   -1325, 264,   383,   -829,  1458,  -1602, -130,
    -681,  1017,  732,   608,   -1542, 411,   -205,  -1571,
    1223,  652,   -552,  1015,  -1293, 1491,  -282,  -1544,
    516,   -8,    -320,  -666,  -1618, -1162, 126,   1469,
    -853,  -90,   -271,  830,   107,   -1421, -247,  -951,
    -398,  961,   -1508, -725,  448,   -1065, 677,   -1275,
    -1103, 430,   555,   843,   -1251, 871,   1550,  105,
    422,   587,   177,   -235,  -291,  -460,  1574,  1653,
    -246,  778,   1159,  -147,  -777,  1483,  -602,  1119,
    -1590, 644,   -872,  349,   418,   329,   -156,  -75,
    817,   1097,  603,   610,   1322,  -1285, -1465, 384,
    -1215, -136,  1218,  -1335, -874,  220,   -1187, -1659,
    -1185, -1530, -1278, 794,   -1510, -854,  -870,  478,
    -108,  -308,  996,   991,   958,   -1460, 1522,  1628
};
// clang-format on

/*********************
 *  STATIC FUNCTIONS
 *********************/

/**
 * @brief   Montgomery reduction mod 3329.
 */
static inline int16_t mldsa_small_montgomery_reduce(int32_t a)
{
    int16_t t;
    t = (int16_t) ((int16_t) a * (int16_t) MLDSA_SMALL_QINV);
    t = (int16_t) ((a - (int32_t) t * MLDSA_SMALL_Q) >> 16);
    return t;
}

/**
 * @brief   Barrett reduction mod 3329.
 */
static inline int16_t mldsa_small_barrett_reduce(int16_t a)
{
    int16_t       t;
    const int16_t v = ((1 << 26) + MLDSA_SMALL_Q / 2) / MLDSA_SMALL_Q;
    t               = (int16_t) (((int32_t) v * a + (1 << 25)) >> 26);
    t               = (int16_t) (t * MLDSA_SMALL_Q);
    return (int16_t) (a - t);
}

/**
 * @brief   Multiplication followed by Montgomery reduction mod 3329.
 */
static inline int16_t mldsa_small_fqmul(int16_t a, int16_t b)
{
    return mldsa_small_montgomery_reduce((int32_t) a * b);
}

/*********************
 *  GLOBAL FUNCTIONS
 *********************/

void MLDSA_SMALLPOLY_ntt(int16_t r[MLDSA_N])
{
    uint32_t len, start, j, k;
    int16_t  t, zeta;

    k = 1U;
    for (len = 128U; len >= 2U; len >>= 1U) {
        for (start = 0U; start < MLDSA_N; start = j + len) {
            zeta = mldsa_small_zetas[k++];
            for (j = start; j < start + len; j++) {
                t          = mldsa_small_fqmul(zeta, r[j + len]);
                r[j + len] = (int16_t) (r[j] - t);
                r[j]       = (int16_t) (r[j] + t);
            }
        }
    }
}

void MLDSA_SMALLPOLY_invntt_tomont(int16_t r[MLDSA_N])
{
    uint32_t      start, len, j, k;
    int16_t       t, zeta;
    const int16_t f = 1441;  // mont^2 / 128

    k = 127U;
    for (len = 2U; len <= 128U; len <<= 1U) {
        for (start = 0U; start < MLDSA_N; start = j + len) {
            zeta = mldsa_small_zetas[k--];
            for (j = start; j < start + len; j++) {
                t          = r[j];
                r[j]       = mldsa_small_barrett_reduce((int16_t) (t + r[j + len]));
                r[j + len] = (int16_t) (r[j + len] - t);
                r[j + len] = mldsa_small_fqmul(zeta, r[j + len]);
            }
        }
    }

    for (j = 0U; j < MLDSA_N; j++) {
        r[j] = mldsa_small_barrett_reduce(mldsa_small_fqmul(r[j], f));
    }
}

/**
 * @brief   Basemul for two pairs of coefficients in NTT domain.
 */
static inline void mldsa_small_basemul(int16_t       r[2],
                                       const int16_t a[2],
                                       const int16_t b[2],
                                       int16_t       zeta)
{
    int16_t a0 = a[0], a1 = a[1];
    int16_t b0 = b[0];

    r[0] = mldsa_small_fqmul(a1, b[1]);
    r[0] = mldsa_small_fqmul(r[0], zeta);
    r[0] = (int16_t) (r[0] + mldsa_small_fqmul(a0, b0));
    r[1] = mldsa_small_fqmul(a0, b[1]);
    r[1] = (int16_t) (r[1] + mldsa_small_fqmul(a1, b0));
}

void MLDSA_SMALLPOLY_ntt_copy(mldsa_smallpoly *out, const mldsa_poly *in)
{
    for (uint32_t i = 0U; i < MLDSA_N; i++) {
        out->coeffs[i] = (int16_t) in->coeffs[i];
    }
    MLDSA_SMALLPOLY_ntt(out->coeffs);
}

void MLDSA_SMALLPOLY_basemul_invntt(mldsa_poly            *r,
                                    const mldsa_smallpoly *a,
                                    const mldsa_smallpoly *b)
{
    // Re-use the output buffer as a smallpoly during computation
    mldsa_smallpoly *tmp = (mldsa_smallpoly *) r;

    // Basemul in NTT domain
    for (uint32_t i = 0U; i < MLDSA_N / 4U; i++) {
        mldsa_small_basemul(&tmp->coeffs[4U * i],
                            &a->coeffs[4U * i],
                            &b->coeffs[4U * i],
                            mldsa_small_zetas[64U + i]);
        mldsa_small_basemul(&tmp->coeffs[4U * i + 2U],
                            &a->coeffs[4U * i + 2U],
                            &b->coeffs[4U * i + 2U],
                            (int16_t) (-mldsa_small_zetas[64U + i]));
    }

    // Inverse NTT
    MLDSA_SMALLPOLY_invntt_tomont(tmp->coeffs);

    // Expand int16_t to int32_t in reverse order to avoid clobbering
    for (int32_t j = (int32_t) MLDSA_N - 1; j >= 0; j--) {
        r->coeffs[j] = (int32_t) tmp->coeffs[j];
    }
}

void MLDSA_SMALLPOLY_unpack_eta(mldsa_smallpoly *r, const uint8_t *a, uint8_t eta)
{
    if (eta == 2U) {
        for (uint32_t i = 0U; i < MLDSA_N / 8U; i++) {
            r->coeffs[8U * i + 0U] = (int16_t) ((a[3U * i + 0U] >> 0U) & 7U);
            r->coeffs[8U * i + 1U] = (int16_t) ((a[3U * i + 0U] >> 3U) & 7U);
            r->coeffs[8U * i + 2U]
                = (int16_t) (((a[3U * i + 0U] >> 6U) | (a[3U * i + 1U] << 2U)) & 7U);
            r->coeffs[8U * i + 3U] = (int16_t) ((a[3U * i + 1U] >> 1U) & 7U);
            r->coeffs[8U * i + 4U] = (int16_t) ((a[3U * i + 1U] >> 4U) & 7U);
            r->coeffs[8U * i + 5U]
                = (int16_t) (((a[3U * i + 1U] >> 7U) | (a[3U * i + 2U] << 1U)) & 7U);
            r->coeffs[8U * i + 6U] = (int16_t) ((a[3U * i + 2U] >> 2U) & 7U);
            r->coeffs[8U * i + 7U] = (int16_t) ((a[3U * i + 2U] >> 5U) & 7U);

            r->coeffs[8U * i + 0U] = (int16_t) eta - r->coeffs[8U * i + 0U];
            r->coeffs[8U * i + 1U] = (int16_t) eta - r->coeffs[8U * i + 1U];
            r->coeffs[8U * i + 2U] = (int16_t) eta - r->coeffs[8U * i + 2U];
            r->coeffs[8U * i + 3U] = (int16_t) eta - r->coeffs[8U * i + 3U];
            r->coeffs[8U * i + 4U] = (int16_t) eta - r->coeffs[8U * i + 4U];
            r->coeffs[8U * i + 5U] = (int16_t) eta - r->coeffs[8U * i + 5U];
            r->coeffs[8U * i + 6U] = (int16_t) eta - r->coeffs[8U * i + 6U];
            r->coeffs[8U * i + 7U] = (int16_t) eta - r->coeffs[8U * i + 7U];
        }
    }
    else {
        // eta == 4
        for (uint32_t i = 0U; i < MLDSA_N / 2U; i++) {
            r->coeffs[2U * i + 0U] = (int16_t) (a[i] & 0x0FU);
            r->coeffs[2U * i + 1U] = (int16_t) (a[i] >> 4U);
            r->coeffs[2U * i + 0U] = (int16_t) eta - r->coeffs[2U * i + 0U];
            r->coeffs[2U * i + 1U] = (int16_t) eta - r->coeffs[2U * i + 1U];
        }
    }
}

#endif /* HAVE_MLDSA_OPTIMIZATION */
