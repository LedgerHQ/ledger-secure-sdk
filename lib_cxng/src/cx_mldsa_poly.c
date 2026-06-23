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
 * @file    cx_mldsa_poly.c
 * @brief   ML-DSA polynomial arithmetic (FIPS 204).
 *
 * Implements NTT, inverse NTT, Montgomery reduction, pointwise multiplication,
 * and related polynomial operations for the ML-DSA ring Z_q[X]/(X^256+1).
 */

#include "cx_mldsa_poly.h"

// clang-format off
static const int32_t mldsa_zetas[MLDSA_N] = {
    0,        25847,    -2608894, -518909,  237124,   -777960,  -876248,
    466468,   1826347,  2353451,  -359251,  -2091905, 3119733,  -2884855,
    3111497,  2680103,  2725464,  1024112,  -1079900, 3585928,  -549488,
    -1119584, 2619752,  -2108549, -2118186, -3859737, -1399561, -3277672,
    1757237,  -19422,   4010497,  280005,   2706023,  95776,    3077325,
    3530437,  -1661693, -3592148, -2537516, 3915439,  -3861115, -3043716,
    3574422,  -2867647, 3539968,  -300467,  2348700,  -539299,  -1699267,
    -1643818, 3505694,  -3821735, 3507263,  -2140649, -1600420, 3699596,
    811944,   531354,   954230,   3881043,  3900724,  -2556880, 2071892,
    -2797779, -3930395, -1528703, -3677745, -3041255, -1452451, 3475950,
    2176455,  -1585221, -1257611, 1939314,  -4083598, -1000202, -3190144,
    -3157330, -3632928, 126922,   3412210,  -983419,  2147896,  2715295,
    -2967645, -3693493, -411027,  -2477047, -671102,  -1228525, -22981,
    -1308169, -381987,  1349076,  1852771,  -1430430, -3343383, 264944,
    508951,   3097992,  44288,    -1100098, 904516,   3958618,  -3724342,
    -8578,    1653064,  -3249728, 2389356,  -210977,  759969,   -1316856,
    189548,   -3553272, 3159746,  -1851402, -2409325, -177440,  1315589,
    1341330,  1285669,  -1584928, -812732,  -1439742, -3019102, -3881060,
    -3628969, 3839961,  2091667,  3407706,  2316500,  3817976,  -3342478,
    2244091,  -2446433, -3562462, 266997,   2434439,  -1235728, 3513181,
    -3520352, -3759364, -1197226, -3193378, 900702,   1859098,  909542,
    819034,   495491,   -1613174, -43260,   -522500,  -655327,  -3122442,
    2031748,  3207046,  -3556995, -525098,  -768622,  -3595838, 342297,
    286988,   -2437823, 4108315,  3437287,  -3342277, 1735879,  203044,
    2842341,  2691481,  -2590150, 1265009,  4055324,  1247620,  2486353,
    1595974,  -3767016, 1250494,  2635921,  -3548272, -2994039, 1869119,
    1903435,  -1050970, -1333058, 1237275,  -3318210, -1430225, -451100,
    1312455,  3306115,  -1962642, -1279661, 1917081,  -2546312, -1374803,
    1500165,  777191,   2235880,  3406031,  -542412,  -2831860, -1671176,
    -1846953, -2584293, -3724270, 594136,   -3776993, -2013608, 2432395,
    2454455,  -164721,  1957272,  3369112,  185531,   -1207385, -3183426,
    162844,   1616392,  3014001,  810149,   1652634,  -3694233, -1799107,
    -3038916, 3523897,  3866901,  269760,   2213111,  -975884,  1717735,
    472078,   -426683,  1723600,  -1803090, 1910376,  -1667432, -1104333,
    -260646,  -3833893, -2939036, -2235985, -420899,  -2286327, 183443,
    -976891,  1612842,  -3545687, -554416,  3919660,  -48306,   -1362209,
    3937738,  1400424,  -846154,  1976782,
};
// clang-format on

int32_t MLDSA_POLY_montgomery_reduce(int64_t a)
{
    int32_t t;
    t = (int32_t) ((int64_t) (int32_t) a * (int64_t) MLDSA_QINV);
    t = (int32_t) ((a - (int64_t) t * (int64_t) MLDSA_Q) >> 32);
    return t;
}

int32_t MLDSA_POLY_reduce32(int32_t a)
{
    int32_t t;
    t = (a + (1 << 22)) >> 23;
    t = a - t * MLDSA_Q;
    return t;
}

int32_t MLDSA_POLY_caddq(int32_t a)
{
    a += (a >> 31) & MLDSA_Q;
    return a;
}

void MLDSA_POLY_reduce(mldsa_poly *a)
{
    for (uint32_t i = 0U; i < MLDSA_N; i++) {
        a->coeffs[i] = MLDSA_POLY_reduce32(a->coeffs[i]);
    }
}

void MLDSA_POLY_caddq_all(mldsa_poly *a)
{
    for (uint32_t i = 0U; i < MLDSA_N; i++) {
        a->coeffs[i] = MLDSA_POLY_caddq(a->coeffs[i]);
    }
}

void MLDSA_POLY_add(mldsa_poly *a, const mldsa_poly *b)
{
    for (uint32_t i = 0U; i < MLDSA_N; i++) {
        a->coeffs[i] += b->coeffs[i];
    }
}

void MLDSA_POLY_sub(mldsa_poly *a, const mldsa_poly *b)
{
    for (uint32_t i = 0U; i < MLDSA_N; i++) {
        a->coeffs[i] -= b->coeffs[i];
    }
}

void MLDSA_POLY_shiftl(mldsa_poly *a)
{
    for (uint32_t i = 0U; i < MLDSA_N; i++) {
        a->coeffs[i] <<= MLDSA_D;
    }
}

void MLDSA_POLY_ntt(mldsa_poly *a)
{
    uint32_t k = 0U;
    for (uint32_t len = 128U; len >= 1U; len >>= 1U) {
        for (uint32_t start = 0U; start < MLDSA_N; start += 2U * len) {
            k++;
            int32_t zeta = mldsa_zetas[k];
            for (uint32_t j = start; j < start + len; j++) {
                int32_t t = MLDSA_POLY_montgomery_reduce((int64_t) zeta * a->coeffs[j + len]);
                a->coeffs[j + len] = a->coeffs[j] - t;
                a->coeffs[j]       = a->coeffs[j] + t;
            }
        }
    }
}

void MLDSA_POLY_invntt_tomont(mldsa_poly *a)
{
    uint32_t k = MLDSA_N;
    int32_t  f = 41978;  // mont^2 / 256
    for (uint32_t len = 1U; len < MLDSA_N; len <<= 1U) {
        for (uint32_t start = 0U; start < MLDSA_N; start += 2U * len) {
            k--;
            int32_t zeta = -mldsa_zetas[k];
            for (uint32_t j = start; j < start + len; j++) {
                int32_t t          = a->coeffs[j];
                a->coeffs[j]       = t + a->coeffs[j + len];
                a->coeffs[j + len] = t - a->coeffs[j + len];
                a->coeffs[j + len]
                    = MLDSA_POLY_montgomery_reduce((int64_t) zeta * a->coeffs[j + len]);
            }
        }
    }
    for (uint32_t j = 0U; j < MLDSA_N; j++) {
        a->coeffs[j] = MLDSA_POLY_montgomery_reduce((int64_t) f * a->coeffs[j]);
    }
}

void MLDSA_POLY_pointwise_montgomery(mldsa_poly       *c,
                                     const mldsa_poly *a,
                                     const mldsa_poly *b,
                                     int               first)
{
    for (uint32_t i = 0U; i < MLDSA_N; i++) {
        int32_t t = MLDSA_POLY_montgomery_reduce((int64_t) a->coeffs[i] * b->coeffs[i]);
        if (first) {
            c->coeffs[i] = t;
        }
        else {
            c->coeffs[i] += t;
        }
    }
}

int MLDSA_POLY_chknorm(const mldsa_poly *a, int32_t B)
{
    if (B > (MLDSA_Q - 1) / 8) {
        return 1;
    }
    for (uint32_t i = 0U; i < MLDSA_N; i++) {
        int32_t t = a->coeffs[i] >> 31;
        t         = a->coeffs[i] - (t & (2 * a->coeffs[i]));
        if (t >= B) {
            return 1;
        }
    }
    return 0;
}
