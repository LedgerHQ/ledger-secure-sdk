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
 * @file    cx_mlkem_poly.c
 * @brief   ML-KEM polynomial operations (FIPS 203).
 *
 * Implements polynomial arithmetic (NTT, inverse NTT, base multiplication,
 * Barrett/Montgomery reduction), serialization, compression/decompression,
 * message encoding/decoding, and sampling routines.
 */

#include "lcx_mlkem.h"
#include "lcx_sha3.h"
#include "cx_mlkem_poly.h"
#include "cx_mlkem_util.h"

/* ========================================================================
 * ML-KEM Arithmetic Constants
 * ====================================================================== */

#define MLKEM_QINV          62209U /**< q^{-1} mod 2^{16} (Montgomery inverse).     */
#define MLKEM_BARRETT_V     20159  /**< Barrett multiplier for q = 3329.             */
#define MLKEM_BARRETT_SHIFT 26     /**< Barrett right-shift amount.                  */
#define MLKEM_MONT_FACTOR   1353   /**< 2^{32} mod q  (to-Montgomery factor).       */
#define MLKEM_INVNTT_SCALE  1441   /**< 128^{-1} * 2^{16} mod q  (inv-NTT scale).   */

/* ========================================================================
 * ML-KEM Scalar Compression Constants
 * ====================================================================== */

#define MLKEM_COMPRESS_CONST_D1 1290168U /**< Compression multiplier for d = 1.           */
#define MLKEM_COMPRESS_CONST_D4 1290160U /**< Compression multiplier for d = 4.           */
#define MLKEM_COMPRESS_CONST_D5 1290176U /**< Compression multiplier for d = 5.           */
#define MLKEM_COMPRESS_CONST_D10_11 \
    2642263040ULL /**< Compression multiplier for d = 10 and 11.   */

/* ========================================================================
 * Polynomial Arithmetic
 * ====================================================================== */

static const int16_t zetas[MLKEM_N / 2U] = {
    -1044, -758,  -359,  -1517, 1493,  1422,  287,   202,   -171,  622,   1577,  182,   962,
    -1202, -1474, 1468,  573,   -1325, 264,   383,   -829,  1458,  -1602, -130,  -681,  1017,
    732,   608,   -1542, 411,   -205,  -1571, 1223,  652,   -552,  1015,  -1293, 1491,  -282,
    -1544, 516,   -8,    -320,  -666,  -1618, -1162, 126,   1469,  -853,  -90,   -271,  830,
    107,   -1421, -247,  -951,  -398,  961,   -1508, -725,  448,   -1065, 677,   -1275, -1103,
    430,   555,   843,   -1251, 871,   1550,  105,   422,   587,   177,   -235,  -291,  -460,
    1574,  1653,  -246,  778,   1159,  -147,  -777,  1483,  -602,  1119,  -1590, 644,   -872,
    349,   418,   329,   -156,  -75,   817,   1097,  603,   610,   1322,  -1285, -1465, 384,
    -1215, -136,  1218,  -1335, -874,  220,   -1187, -1659, -1185, -1530, -1278, 794,   -1510,
    -854,  -870,  478,   -108,  -308,  996,   991,   958,   -1460, 1522,  1628,
};

int16_t MLKEM_POLY_montgomery_reduce(int32_t a)
{
    uint16_t a_lo = (uint16_t) a;
    int16_t  t    = (int16_t) ((uint16_t) (a_lo * MLKEM_QINV));
    int32_t  r    = a - ((int32_t) t * MLKEM_Q);
    return (int16_t) (r >> 16);
}

int16_t MLKEM_POLY_fqmul(int16_t a, int16_t b)
{
    return MLKEM_POLY_montgomery_reduce((int32_t) a * (int32_t) b);
}

int16_t MLKEM_POLY_barrett_reduce(int16_t a)
{
    int32_t t = ((MLKEM_BARRETT_V * (int32_t) a) + (1 << (MLKEM_BARRETT_SHIFT - 1)))
                >> MLKEM_BARRETT_SHIFT;
    return (int16_t) ((int32_t) a - (t * MLKEM_Q));
}

int16_t MLKEM_POLY_signed_to_unsigned_q(int16_t c)
{
    int16_t mask = (int16_t) (c >> 15);
    return (int16_t) (c + (mask & MLKEM_Q));
}

void MLKEM_POLY_reduce(poly *r)
{
    for (uint32_t i = 0U; i < MLKEM_N; i++) {
        r->coeffs[i] = MLKEM_POLY_signed_to_unsigned_q(MLKEM_POLY_barrett_reduce(r->coeffs[i]));
    }
}

void MLKEM_POLY_tomont(poly *r)
{
    for (uint32_t i = 0U; i < MLKEM_N; i++) {
        r->coeffs[i] = MLKEM_POLY_fqmul(r->coeffs[i], MLKEM_MONT_FACTOR);
    }
}

void MLKEM_POLY_add(poly *r, const poly *b)
{
    for (uint32_t i = 0U; i < MLKEM_N; i++) {
        r->coeffs[i] = (int16_t) (r->coeffs[i] + b->coeffs[i]);
    }
}

void MLKEM_POLY_sub(poly *r, const poly *b)
{
    for (uint32_t i = 0U; i < MLKEM_N; i++) {
        r->coeffs[i] = (int16_t) (r->coeffs[i] - b->coeffs[i]);
    }
}

void MLKEM_POLY_ntt(poly *p)
{
    int16_t *r = p->coeffs;
    uint32_t k = 1U;

    for (uint32_t len = MLKEM_N / 2U; len >= 2U; len >>= 1U) {
        for (uint32_t start = 0U; start < MLKEM_N; start += 2U * len) {
            int16_t zeta = zetas[k++];
            for (uint32_t j = start; j < (start + len); j++) {
                int16_t t  = MLKEM_POLY_fqmul(r[j + len], zeta);
                r[j + len] = (int16_t) (r[j] - t);
                r[j]       = (int16_t) (r[j] + t);
            }
        }
    }
}

void MLKEM_POLY_invntt_tomont(poly *p)
{
    int16_t *r = p->coeffs;
    uint32_t k = (MLKEM_N / 2U) - 1U;

    for (uint32_t j = 0U; j < MLKEM_N; j++) {
        r[j] = MLKEM_POLY_fqmul(r[j], MLKEM_INVNTT_SCALE);
    }

    for (uint32_t len = 2U; len <= MLKEM_N / 2U; len <<= 1U) {
        for (uint32_t start = 0U; start < MLKEM_N; start += 2U * len) {
            int16_t zeta = zetas[k--];
            for (uint32_t j = start; j < (start + len); j++) {
                int16_t t  = r[j];
                r[j]       = MLKEM_POLY_barrett_reduce((int16_t) (t + r[j + len]));
                r[j + len] = MLKEM_POLY_fqmul((int16_t) (r[j + len] - t), zeta);
            }
        }
    }
}

void MLKEM_POLY_basemul_acc_montgomery(poly *r, const poly *a, const poly *b, int32_t first)
{
    for (uint32_t i = 0U; i < (MLKEM_N / 4U); i++) {
        int16_t zeta = zetas[(MLKEM_N / 4U) + i];

        int32_t t0
            = (int32_t) a->coeffs[(4U * i) + 1U] * MLKEM_POLY_fqmul(b->coeffs[(4U * i) + 1U], zeta);
        t0 += (int32_t) a->coeffs[4U * i] * b->coeffs[4U * i];

        int32_t t1 = (int32_t) a->coeffs[4U * i] * b->coeffs[(4U * i) + 1U];
        t1 += (int32_t) a->coeffs[(4U * i) + 1U] * b->coeffs[4U * i];

        if (first != 0) {
            r->coeffs[4U * i]        = MLKEM_POLY_montgomery_reduce(t0);
            r->coeffs[(4U * i) + 1U] = MLKEM_POLY_montgomery_reduce(t1);
        }
        else {
            r->coeffs[4U * i] = (int16_t) (r->coeffs[4U * i] + MLKEM_POLY_montgomery_reduce(t0));
            r->coeffs[(4U * i) + 1U]
                = (int16_t) (r->coeffs[(4U * i) + 1U] + MLKEM_POLY_montgomery_reduce(t1));
        }

        int16_t neg_zeta = (int16_t) (-zeta);
        t0               = (int32_t) a->coeffs[(4U * i) + 3U]
             * MLKEM_POLY_fqmul(b->coeffs[(4U * i) + 3U], neg_zeta);
        t0 += (int32_t) a->coeffs[(4U * i) + 2U] * b->coeffs[(4U * i) + 2U];

        t1 = (int32_t) a->coeffs[(4U * i) + 2U] * b->coeffs[(4U * i) + 3U];
        t1 += (int32_t) a->coeffs[(4U * i) + 3U] * b->coeffs[(4U * i) + 2U];

        if (first != 0) {
            r->coeffs[(4U * i) + 2U] = MLKEM_POLY_montgomery_reduce(t0);
            r->coeffs[(4U * i) + 3U] = MLKEM_POLY_montgomery_reduce(t1);
        }
        else {
            r->coeffs[(4U * i) + 2U]
                = (int16_t) (r->coeffs[(4U * i) + 2U] + MLKEM_POLY_montgomery_reduce(t0));
            r->coeffs[(4U * i) + 3U]
                = (int16_t) (r->coeffs[(4U * i) + 3U] + MLKEM_POLY_montgomery_reduce(t1));
        }
    }
}

/* ========================================================================
 * Polynomial Serialization
 * ====================================================================== */

void MLKEM_POLY_tobytes(uint8_t *r, const poly *a)
{
    for (uint32_t i = 0U; i < (MLKEM_N / 2U); i++) {
        uint16_t t0      = (uint16_t) a->coeffs[2U * i];
        uint16_t t1      = (uint16_t) a->coeffs[(2U * i) + 1U];
        r[(3U * i) + 0U] = (uint8_t) (t0 & 0xFFU);
        r[(3U * i) + 1U] = (uint8_t) ((t0 >> 8U) | ((t1 << 4U) & 0xF0U));
        r[(3U * i) + 2U] = (uint8_t) (t1 >> 4U);
    }
}

void MLKEM_POLY_frombytes(poly *r, const uint8_t *a)
{
    for (uint32_t i = 0U; i < (MLKEM_N / 2U); i++) {
        uint8_t t0               = a[(3U * i) + 0U];
        uint8_t t1               = a[(3U * i) + 1U];
        uint8_t t2               = a[(3U * i) + 2U];
        r->coeffs[(2U * i) + 0U] = (int16_t) (t0 | (((uint16_t) t1 << 8U) & 0xFFFU));
        r->coeffs[(2U * i) + 1U] = (int16_t) ((t1 >> 4U) | ((uint16_t) t2 << 4U));
    }
}

/* ========================================================================
 * Compression Functions
 * ====================================================================== */

uint8_t MLKEM_POLY_scalar_compress_d1(int16_t u)
{
    uint32_t d0 = (uint32_t) u * MLKEM_COMPRESS_CONST_D1;
    return (uint8_t) ((d0 + ((uint32_t) 1u << 30)) >> 31);
}

uint8_t MLKEM_POLY_scalar_compress_d4(int16_t u)
{
    uint32_t d0 = (uint32_t) u * MLKEM_COMPRESS_CONST_D4;
    return (uint8_t) ((d0 + ((uint32_t) 1u << 27)) >> 28);
}

int16_t MLKEM_POLY_scalar_decompress_d4(uint8_t u)
{
    return (int16_t) ((((uint32_t) u * MLKEM_Q) + 8) >> 4);
}

uint8_t MLKEM_POLY_scalar_compress_d5(int16_t u)
{
    uint32_t d0 = (uint32_t) u * MLKEM_COMPRESS_CONST_D5;
    return (uint8_t) ((d0 + ((uint32_t) 1u << 26)) >> 27);
}

int16_t MLKEM_POLY_scalar_decompress_d5(uint8_t u)
{
    return (int16_t) ((((uint32_t) u * MLKEM_Q) + 16) >> 5);
}

uint16_t MLKEM_POLY_scalar_compress_d10(int16_t u)
{
    uint64_t d0 = (uint64_t) u * MLKEM_COMPRESS_CONST_D10_11;
    d0          = (d0 + ((uint64_t) 1u << 32)) >> 33;
    return (uint16_t) (d0 & 0x3FF);
}

int16_t MLKEM_POLY_scalar_decompress_d10(uint16_t u)
{
    return (int16_t) ((((uint32_t) u * MLKEM_Q) + 512) >> 10);
}

uint16_t MLKEM_POLY_scalar_compress_d11(int16_t u)
{
    uint64_t d0 = (uint64_t) u * MLKEM_COMPRESS_CONST_D10_11;
    d0          = (d0 + ((uint64_t) 1u << 31)) >> 32;
    return (uint16_t) (d0 & 0x7FF);
}

int16_t MLKEM_POLY_scalar_decompress_d11(uint16_t u)
{
    return (int16_t) ((((uint32_t) u * MLKEM_Q) + 1024) >> 11);
}

/* Compress dv=4 */
void MLKEM_POLY_compress_d4(uint8_t *r, const poly *a)
{
    for (uint32_t i = 0U; i < (MLKEM_N / 8U); i++) {
        uint8_t t[8] = {0};
        for (uint32_t j = 0U; j < 8U; j++) {
            t[j] = MLKEM_POLY_scalar_compress_d4(a->coeffs[(8U * i) + j]);
        }
        r[(i * 4U) + 0U] = (uint8_t) (t[0] | (uint8_t) (t[1] << 4U));
        r[(i * 4U) + 1U] = (uint8_t) (t[2] | (uint8_t) (t[3] << 4U));
        r[(i * 4U) + 2U] = (uint8_t) (t[4] | (uint8_t) (t[5] << 4U));
        r[(i * 4U) + 3U] = (uint8_t) (t[6] | (uint8_t) (t[7] << 4U));
    }
}

void MLKEM_POLY_decompress_d4(poly *r, const uint8_t *a)
{
    for (uint32_t i = 0U; i < (MLKEM_N / 2U); i++) {
        r->coeffs[(2U * i) + 0U] = MLKEM_POLY_scalar_decompress_d4((a[i] >> 0U) & 0xFU);
        r->coeffs[(2U * i) + 1U] = MLKEM_POLY_scalar_decompress_d4((a[i] >> 4U) & 0xFU);
    }
}

/* Compress dv=5 */
void MLKEM_POLY_compress_d5(uint8_t *r, const poly *a)
{
    for (uint32_t i = 0U; i < (MLKEM_N / 8U); i++) {
        uint8_t t[8] = {0};
        for (uint32_t j = 0U; j < 8U; j++) {
            t[j] = MLKEM_POLY_scalar_compress_d5(a->coeffs[(8U * i) + j]);
        }
        r[(5U * i) + 0U] = (uint8_t) ((t[0] >> 0) | (t[1] << 5));
        r[(5U * i) + 1U] = (uint8_t) ((t[1] >> 3) | (t[2] << 2) | (t[3] << 7));
        r[(5U * i) + 2U] = (uint8_t) ((t[3] >> 1) | (t[4] << 4));
        r[(5U * i) + 3U] = (uint8_t) ((t[4] >> 4) | (t[5] << 1) | (t[6] << 6));
        r[(5U * i) + 4U] = (uint8_t) ((t[6] >> 2) | (t[7] << 3));
    }
}

void MLKEM_POLY_decompress_d5(poly *r, const uint8_t *a)
{
    for (uint32_t i = 0U; i < (MLKEM_N / 8U); i++) {
        uint8_t t[8] = {0};
        t[0]         = (a[5 * i + 0] >> 0) & 0x1F;
        t[1]         = ((a[5 * i + 0] >> 5) | (a[5 * i + 1] << 3)) & 0x1F;
        t[2]         = (a[5 * i + 1] >> 2) & 0x1F;
        t[3]         = ((a[5 * i + 1] >> 7) | (a[5 * i + 2] << 1)) & 0x1F;
        t[4]         = ((a[5 * i + 2] >> 4) | (a[5 * i + 3] << 4)) & 0x1F;
        t[5]         = (a[5 * i + 3] >> 1) & 0x1F;
        t[6]         = ((a[5 * i + 3] >> 6) | (a[5 * i + 4] << 2)) & 0x1F;
        t[7]         = (a[5 * i + 4] >> 3) & 0x1F;
        for (uint32_t j = 0; j < 8; j++) {
            r->coeffs[8 * i + j] = MLKEM_POLY_scalar_decompress_d5(t[j]);
        }
    }
}

/* Compress du=10 */
void MLKEM_POLY_compress_d10(uint8_t *r, const poly *a)
{
    for (uint32_t j = 0U; j < (MLKEM_N / 4U); j++) {
        uint16_t t[4] = {0};
        for (uint32_t k = 0U; k < 4U; k++) {
            t[k] = MLKEM_POLY_scalar_compress_d10(a->coeffs[(4U * j) + k]);
        }
        r[(5U * j) + 0U] = (uint8_t) ((t[0] >> 0U) & 0xFFU);
        r[(5U * j) + 1U] = (uint8_t) ((t[0] >> 8U) | ((t[1] << 2U) & 0xFFU));
        r[(5U * j) + 2U] = (uint8_t) ((t[1] >> 6U) | ((t[2] << 4U) & 0xFFU));
        r[(5U * j) + 3U] = (uint8_t) ((t[2] >> 4U) | ((t[3] << 6U) & 0xFFU));
        r[(5U * j) + 4U] = (uint8_t) (t[3] >> 2U);
    }
}

void MLKEM_POLY_decompress_d10(poly *r, const uint8_t *a)
{
    for (uint32_t j = 0U; j < (MLKEM_N / 4U); j++) {
        const uint8_t *base = &a[5U * j];
        uint16_t       t[4] = {0};
        t[0]                = 0x3FFU & ((base[0] >> 0U) | ((uint16_t) base[1] << 8U));
        t[1]                = 0x3FFU & ((base[1] >> 2U) | ((uint16_t) base[2] << 6U));
        t[2]                = 0x3FFU & ((base[2] >> 4U) | ((uint16_t) base[3] << 4U));
        t[3]                = 0x3FFU & ((base[3] >> 6U) | ((uint16_t) base[4] << 2U));
        for (uint32_t k = 0U; k < 4U; k++) {
            r->coeffs[(4U * j) + k] = MLKEM_POLY_scalar_decompress_d10(t[k]);
        }
    }
}

/* Compress du=11 */
void MLKEM_POLY_compress_d11(uint8_t *r, const poly *a)
{
    for (uint32_t j = 0U; j < (MLKEM_N / 8U); j++) {
        uint16_t t[8] = {0};
        for (uint32_t k = 0U; k < 8U; k++) {
            t[k] = MLKEM_POLY_scalar_compress_d11(a->coeffs[(8U * j) + k]);
        }
        r[(11U * j) + 0U]  = (uint8_t) (t[0] >> 0);
        r[(11U * j) + 1U]  = (uint8_t) ((t[0] >> 8) | (t[1] << 3));
        r[(11U * j) + 2U]  = (uint8_t) ((t[1] >> 5) | (t[2] << 6));
        r[(11U * j) + 3U]  = (uint8_t) (t[2] >> 2);
        r[(11U * j) + 4U]  = (uint8_t) ((t[2] >> 10) | (t[3] << 1));
        r[(11U * j) + 5U]  = (uint8_t) ((t[3] >> 7) | (t[4] << 4));
        r[(11U * j) + 6U]  = (uint8_t) ((t[4] >> 4) | (t[5] << 7));
        r[(11U * j) + 7U]  = (uint8_t) (t[5] >> 1);
        r[(11U * j) + 8U]  = (uint8_t) ((t[5] >> 9) | (t[6] << 2));
        r[(11U * j) + 9U]  = (uint8_t) ((t[6] >> 6) | (t[7] << 5));
        r[(11U * j) + 10U] = (uint8_t) (t[7] >> 3);
    }
}

void MLKEM_POLY_decompress_d11(poly *r, const uint8_t *a)
{
    for (uint32_t j = 0U; j < (MLKEM_N / 8U); j++) {
        const uint8_t *base = &a[11U * j];
        uint16_t       t[8] = {0};
        t[0]                = 0x7FFU & ((base[0] >> 0) | ((uint16_t) base[1] << 8));
        t[1]                = 0x7FFU & ((base[1] >> 3) | ((uint16_t) base[2] << 5));
        t[2] = 0x7FFU & ((base[2] >> 6) | ((uint16_t) base[3] << 2) | ((uint16_t) base[4] << 10));
        t[3] = 0x7FFU & ((base[4] >> 1) | ((uint16_t) base[5] << 7));
        t[4] = 0x7FFU & ((base[5] >> 4) | ((uint16_t) base[6] << 4));
        t[5] = 0x7FFU & ((base[6] >> 7) | ((uint16_t) base[7] << 1) | ((uint16_t) base[8] << 9));
        t[6] = 0x7FFU & ((base[8] >> 2) | ((uint16_t) base[9] << 6));
        t[7] = 0x7FFU & ((base[9] >> 5) | ((uint16_t) base[10] << 3));
        for (uint32_t k = 0U; k < 8U; k++) {
            r->coeffs[(8U * j) + k] = MLKEM_POLY_scalar_decompress_d11(t[k]);
        }
    }
}

/* Message encoding/decoding */
void MLKEM_POLY_frommsg(poly *r, const uint8_t *msg)
{
    for (uint32_t i = 0U; i < (MLKEM_N / 8U); i++) {
        for (uint32_t j = 0U; j < 8U; j++) {
            int16_t bit             = (int16_t) ((msg[i] >> j) & 1U);
            r->coeffs[(8U * i) + j] = (int16_t) ((-bit) & MLKEM_Q_HALF);
        }
    }
}

void MLKEM_POLY_tomsg(uint8_t *msg, const poly *a)
{
    for (uint32_t i = 0U; i < (MLKEM_N / 8U); i++) {
        msg[i] = 0U;
        for (uint32_t j = 0U; j < 8U; j++) {
            uint32_t t = MLKEM_POLY_scalar_compress_d1(a->coeffs[(8U * i) + j]);
            msg[i] |= (uint8_t) (t << j);
        }
    }
}
