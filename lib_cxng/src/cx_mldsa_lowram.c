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
 * @file    cx_mldsa_lowram.c
 * @brief   ML-DSA low-RAM helper implementations.
 *
 * Implements compressed polynomial buffers, schoolbook multiplication from
 * packed format, fused A expansion with pointwise accumulation, streaming
 * gamma1 sampling, and hint index-list processing.
 */

#ifdef HAVE_MLDSA_OPTIMIZATION

#include <string.h>
#include "cx_mldsa_lowram.h"
#include "cx_mldsa_rounding.h"
#include "cx_mldsa_poly.h"
#include "cx_mldsa_sample.h"
#include "cx_mldsa_util.h"

/**
 * @brief   Freeze: fully reduce a coefficient to [0, q).
 */
static inline int32_t mldsa_freeze(int32_t a)
{
    a = MLDSA_POLY_reduce32(a);
    a = MLDSA_POLY_caddq(a);
    return a;
}

/**
 * @brief   Read one t0 coefficient from packed 13-bit format at a given index.
 *
 * 8 coefficients are packed in 13 bytes.
 */
static inline int32_t polyt0_unpack_idx(const uint8_t *t0, uint32_t idx)
{
    int32_t        coeff;
    const uint8_t *p = t0 + 13U * (idx >> 3U);
    uint32_t       r = idx & 7U;

    switch (r) {
        case 0U:
            coeff = (int32_t) p[0];
            coeff |= (int32_t) ((uint32_t) p[1] << 8U);
            break;
        case 1U:
            coeff = (int32_t) (p[1] >> 5U);
            coeff |= (int32_t) ((uint32_t) p[2] << 3U);
            coeff |= (int32_t) ((uint32_t) p[3] << 11U);
            break;
        case 2U:
            coeff = (int32_t) (p[3] >> 2U);
            coeff |= (int32_t) ((uint32_t) p[4] << 6U);
            break;
        case 3U:
            coeff = (int32_t) (p[4] >> 7U);
            coeff |= (int32_t) ((uint32_t) p[5] << 1U);
            coeff |= (int32_t) ((uint32_t) p[6] << 9U);
            break;
        case 4U:
            coeff = (int32_t) (p[6] >> 4U);
            coeff |= (int32_t) ((uint32_t) p[7] << 4U);
            coeff |= (int32_t) ((uint32_t) p[8] << 12U);
            break;
        case 5U:
            coeff = (int32_t) (p[8] >> 1U);
            coeff |= (int32_t) ((uint32_t) p[9] << 7U);
            break;
        case 6U:
            coeff = (int32_t) (p[9] >> 6U);
            coeff |= (int32_t) ((uint32_t) p[10] << 2U);
            coeff |= (int32_t) ((uint32_t) p[11] << 10U);
            break;
        default: /* case 7 */
            coeff = (int32_t) (p[11] >> 3U);
            coeff |= (int32_t) ((uint32_t) p[12] << 5U);
            break;
    }
    coeff &= 0x1FFF;
    return (1 << (MLDSA_D - 1)) - coeff;
}

/**
 * @brief   Read one t1 coefficient from packed 10-bit format at a given index.
 *
 * 4 coefficients are packed in 5 bytes.
 */
static inline int32_t polyt1_unpack_idx(const uint8_t *t1, uint32_t idx)
{
    int32_t        coeff;
    const uint8_t *p = t1 + 5U * (idx >> 2U);
    uint32_t       r = idx & 3U;

    switch (r) {
        case 0U:
            coeff = (int32_t) p[0];
            coeff |= (int32_t) ((uint32_t) p[1] << 8U);
            break;
        case 1U:
            coeff = (int32_t) (p[1] >> 2U);
            coeff |= (int32_t) ((uint32_t) p[2] << 6U);
            break;
        case 2U:
            coeff = (int32_t) (p[2] >> 4U);
            coeff |= (int32_t) ((uint32_t) p[3] << 4U);
            break;
        default: /* case 3 */
            coeff = (int32_t) (p[3] >> 6U);
            coeff |= (int32_t) ((uint32_t) p[4] << 2U);
            break;
    }
    coeff &= 0x3FF;
    return coeff;
}

/*********************
 *  COMPRESSED W OPS
 *********************/

void MLDSA_LOWRAM_polyw_pack(uint8_t buf[MLDSA_WCOMP_BYTES], const mldsa_poly *w)
{
    for (uint32_t i = 0U; i < MLDSA_N; i++) {
        int32_t c        = mldsa_freeze(w->coeffs[i]);
        buf[i * 3U + 0U] = (uint8_t) c;
        buf[i * 3U + 1U] = (uint8_t) (c >> 8U);
        buf[i * 3U + 2U] = (uint8_t) (c >> 16U);
    }
}

void MLDSA_LOWRAM_polyw_unpack(mldsa_poly *w, const uint8_t buf[MLDSA_WCOMP_BYTES])
{
    for (uint32_t i = 0U; i < MLDSA_N; i++) {
        w->coeffs[i] = (int32_t) buf[i * 3U + 0U];
        w->coeffs[i] |= (int32_t) ((uint32_t) buf[i * 3U + 1U] << 8U);
        w->coeffs[i] |= (int32_t) ((uint32_t) buf[i * 3U + 2U] << 16U);
    }
}

void MLDSA_LOWRAM_polyw_add_idx(uint8_t buf[MLDSA_WCOMP_BYTES], int32_t a, uint32_t idx)
{
    int32_t coeff;
    coeff = (int32_t) buf[idx * 3U + 0U];
    coeff |= (int32_t) ((uint32_t) buf[idx * 3U + 1U] << 8U);
    coeff |= (int32_t) ((uint32_t) buf[idx * 3U + 2U] << 16U);

    coeff += a;
    coeff = mldsa_freeze(coeff);

    buf[idx * 3U + 0U] = (uint8_t) coeff;
    buf[idx * 3U + 1U] = (uint8_t) (coeff >> 8U);
    buf[idx * 3U + 2U] = (uint8_t) (coeff >> 16U);
}

void MLDSA_LOWRAM_polyw_sub(mldsa_poly       *c,
                            const uint8_t     buf[MLDSA_WCOMP_BYTES],
                            const mldsa_poly *a)
{
    for (uint32_t i = 0U; i < MLDSA_N; i++) {
        int32_t coeff;
        coeff = (int32_t) buf[i * 3U + 0U];
        coeff |= (int32_t) ((uint32_t) buf[i * 3U + 1U] << 8U);
        coeff |= (int32_t) ((uint32_t) buf[i * 3U + 2U] << 16U);
        c->coeffs[i] = coeff - a->coeffs[i];
    }
}

/*********************
 *  CHALLENGE COMPRESS
 *********************/

void MLDSA_LOWRAM_challenge_compress(uint8_t           ccomp[MLDSA_CCOMP_BYTES],
                                     const mldsa_poly *cp,
                                     uint8_t           tau)
{
    uint64_t signs = 0U;
    uint64_t mask  = 1U;
    uint32_t pos   = 0U;

    memset(ccomp, 0, MLDSA_CCOMP_BYTES);

    for (uint32_t i = 0U; i < MLDSA_N; i++) {
        if (cp->coeffs[i] != 0) {
            ccomp[pos++] = (uint8_t) i;
            if (cp->coeffs[i] == -1) {
                signs |= mask;
            }
            mask <<= 1U;
        }
    }
    (void) tau;  // pos should equal tau

    for (uint32_t i = 0U; i < 8U; i++) {
        ccomp[60U + i] = (uint8_t) (signs >> (8U * i));
    }
}

void MLDSA_LOWRAM_challenge_decompress(mldsa_poly   *cp,
                                       const uint8_t ccomp[MLDSA_CCOMP_BYTES],
                                       uint8_t       tau)
{
    uint64_t signs = 0U;

    for (uint32_t i = 0U; i < MLDSA_N; i++) {
        cp->coeffs[i] = 0;
    }
    for (uint32_t i = 0U; i < 8U; i++) {
        signs |= ((uint64_t) ccomp[60U + i]) << (8U * i);
    }
    for (uint32_t i = 0U; i < tau; i++) {
        uint32_t pos = ccomp[i];
        if (signs & 1U) {
            cp->coeffs[pos] = -1;
        }
        else {
            cp->coeffs[pos] = 1;
        }
        signs >>= 1U;
    }
}

/*********************
 *  SCHOOLBOOK OPS
 *********************/

void MLDSA_LOWRAM_schoolbook_t0(mldsa_poly    *c,
                                const uint8_t  ccomp[MLDSA_CCOMP_BYTES],
                                const uint8_t *t0,
                                uint8_t        tau)
{
    uint64_t signs = 0U;

    for (uint32_t i = 0U; i < MLDSA_N; i++) {
        c->coeffs[i] = 0;
    }
    for (uint32_t i = 0U; i < 8U; i++) {
        signs |= ((uint64_t) ccomp[60U + i]) << (8U * i);
    }

    for (uint32_t idx = 0U; idx < tau; idx++) {
        uint32_t pos  = ccomp[idx];
        int32_t  sign = (signs & 1U) ? -1 : 1;

        for (uint32_t j = 0U; pos + j < MLDSA_N; j++) {
            c->coeffs[pos + j] += sign * polyt0_unpack_idx(t0, j);
        }
        for (uint32_t j = MLDSA_N - pos; j < MLDSA_N; j++) {
            c->coeffs[pos + j - MLDSA_N] -= sign * polyt0_unpack_idx(t0, j);
        }
        signs >>= 1U;
    }
}

void MLDSA_LOWRAM_schoolbook_t1(mldsa_poly    *c,
                                const uint8_t  ccomp[MLDSA_CCOMP_BYTES],
                                const uint8_t *t1,
                                uint8_t        tau)
{
    uint64_t signs = 0U;

    for (uint32_t i = 0U; i < MLDSA_N; i++) {
        c->coeffs[i] = 0;
    }
    for (uint32_t i = 0U; i < 8U; i++) {
        signs |= ((uint64_t) ccomp[60U + i]) << (8U * i);
    }

    for (uint32_t idx = 0U; idx < tau; idx++) {
        uint32_t pos  = ccomp[idx];
        int32_t  sign = (signs & 1U) ? -1 : 1;

        for (uint32_t j = 0U; pos + j < MLDSA_N; j++) {
            c->coeffs[pos + j] += sign * (polyt1_unpack_idx(t1, j) << MLDSA_D);
        }
        for (uint32_t j = MLDSA_N - pos; j < MLDSA_N; j++) {
            c->coeffs[pos + j - MLDSA_N] -= sign * (polyt1_unpack_idx(t1, j) << MLDSA_D);
        }
        signs >>= 1U;
    }
}

/*********************
 *  FUSED A EXPANSION
 *********************/

/**
 * Buffer size for A expansion rejection sampling.
 * 5 SHAKE128 blocks = 840 bytes = 280 three-byte candidates.
 * The acceptance rate is q/2^23 ≈ 0.9994, so 280 candidates are more than
 * enough for the 256 coefficients we need.
 */
#define LOWRAM_AEXPAND_NBLOCKS 5U
#define LOWRAM_AEXPAND_BUFSIZE (LOWRAM_AEXPAND_NBLOCKS * 168U)

void MLDSA_LOWRAM_expand_aij_accum(uint8_t           wcomp[MLDSA_WCOMP_BYTES],
                                   const mldsa_poly *b,
                                   const uint8_t     rho[MLDSA_SEEDBYTES],
                                   uint16_t          nonce)
{
    uint8_t  buf[LOWRAM_AEXPAND_BUFSIZE] = {0};
    uint32_t ctr                         = 0U;

    MLDSA_UTIL_shake128_seed_nonce(buf, sizeof(buf), rho, MLDSA_SEEDBYTES, nonce);

    uint32_t pos = 0U;
    while ((ctr < MLDSA_N) && ((pos + 3U) <= sizeof(buf))) {
        uint32_t t;
        t = (uint32_t) buf[pos];
        t |= (uint32_t) buf[pos + 1U] << 8U;
        t |= (uint32_t) buf[pos + 2U] << 16U;
        t &= 0x7FFFFFU;
        pos += 3U;

        if (t < (uint32_t) MLDSA_Q) {
            int32_t mont_prod
                = MLDSA_POLY_montgomery_reduce((int64_t) t * (int64_t) b->coeffs[ctr]);
            MLDSA_LOWRAM_polyw_add_idx(wcomp, mont_prod, ctr);
            ctr++;
        }
    }

    explicit_bzero(buf, sizeof(buf));
}

/*********************
 *  HIGHBITS/LOWBITS
 *********************/

void MLDSA_LOWRAM_poly_highbits(mldsa_poly *a1, const mldsa_poly *a, int32_t gamma2)
{
    int32_t a0_dummy;
    for (uint32_t i = 0U; i < MLDSA_N; i++) {
        a1->coeffs[i] = MLDSA_ROUNDING_decompose(a->coeffs[i], &a0_dummy, gamma2);
    }
}

void MLDSA_LOWRAM_poly_lowbits(mldsa_poly *a0, const mldsa_poly *a, int32_t gamma2)
{
    for (uint32_t i = 0U; i < MLDSA_N; i++) {
        MLDSA_ROUNDING_decompose(a->coeffs[i], &a0->coeffs[i], gamma2);
    }
}

/**
 * Unpack a single coefficient of w1 = HighBits(w) from the packed w1 row.
 */
static int32_t mldsa_lowram_w1_unpack_idx(const uint8_t *w1_packed, uint32_t i, int32_t gamma2)
{
    int32_t w1;
    if (gamma2 == (MLDSA_Q - 1) / 32) {
        // 4 bits per coefficient.
        w1 = (int32_t) ((w1_packed[i >> 1U] >> ((i & 1U) * 4U)) & 0x0FU);
    }
    else {
        // 6 bits per coefficient, 4 coefficients packed in 3 bytes.
        const uint8_t *q = &w1_packed[(i >> 2U) * 3U];
        switch (i & 3U) {
            case 0U:
                w1 = (int32_t) (q[0] & 0x3FU);
                break;
            case 1U:
                w1 = (int32_t) (((uint32_t) (q[0] >> 6U)) | (((uint32_t) (q[1] & 0x0FU)) << 2U));
                break;
            case 2U:
                w1 = (int32_t) (((uint32_t) (q[1] >> 4U)) | (((uint32_t) (q[2] & 0x03U)) << 4U));
                break;
            default:
                w1 = (int32_t) (q[2] >> 2U);
                break;
        }
    }
    return w1;
}

void MLDSA_LOWRAM_poly_r0(mldsa_poly    *r0,
                          const uint8_t  wcomp[MLDSA_WCOMP_BYTES],
                          const uint8_t *w1_packed,
                          int32_t        gamma2)
{
    for (uint32_t i = 0U; i < MLDSA_N; i++) {
        int32_t w_minus_cs2;
        w_minus_cs2 = (int32_t) wcomp[i * 3U + 0U];
        w_minus_cs2 |= (int32_t) ((uint32_t) wcomp[i * 3U + 1U] << 8U);
        w_minus_cs2 |= (int32_t) ((uint32_t) wcomp[i * 3U + 2U] << 16U);

        // r0 = LowBits(w) - c*s2 = (w - c*s2) - HighBits(w) * 2 * gamma2.
        // The high bits are taken from the *original* w (w1_packed), so the
        // result matches the reference even when w - c*s2 crosses a boundary.
        int32_t w1    = mldsa_lowram_w1_unpack_idx(w1_packed, i, gamma2);
        r0->coeffs[i] = MLDSA_POLY_reduce32(w_minus_cs2 - w1 * 2 * gamma2);
    }
}

/*********************
 *  HINT OPERATIONS
 *********************/

uint32_t MLDSA_LOWRAM_make_hint(mldsa_poly       *h,
                                const mldsa_poly *ct0,
                                const uint8_t     wcomp[MLDSA_WCOMP_BYTES],
                                const uint8_t    *w1_packed,
                                int32_t           gamma2)
{
    uint32_t hints_n = 0U;
    for (uint32_t i = 0U; i < MLDSA_N; i++) {
        int32_t w_minus_cs2;
        w_minus_cs2 = (int32_t) wcomp[i * 3U + 0U];
        w_minus_cs2 |= (int32_t) ((uint32_t) wcomp[i * 3U + 1U] << 8U);
        w_minus_cs2 |= (int32_t) ((uint32_t) wcomp[i * 3U + 2U] << 16U);

        // w1 = HighBits(w), unpacked from the packed w1 row. The reference uses
        // the high bits of the *original* w (not w - c*s2): subtracting c*s2 may
        // cross a gamma2 boundary so HighBits(w - c*s2) != HighBits(w).
        int32_t w1 = mldsa_lowram_w1_unpack_idx(w1_packed, i, gamma2);

        // a0 = LowBits(w) - c*s2 + c*t0 = (w - c*s2) + c*t0 - w1 * 2 * gamma2.
        // (w - c*s2) is stored frozen to [0, Q); reduce32 recovers the small
        // centered representative, which equals the reference a0.
        int32_t a0 = MLDSA_POLY_reduce32(w_minus_cs2 + ct0->coeffs[i] - w1 * 2 * gamma2);

        h->coeffs[i] = (int32_t) MLDSA_ROUNDING_make_hint(a0, w1, gamma2);
        if (h->coeffs[i] == 1) {
            hints_n++;
        }
    }
    return hints_n;
}

void MLDSA_LOWRAM_use_hint_indices(mldsa_poly       *b,
                                   const mldsa_poly *a,
                                   const uint8_t    *h_indices,
                                   uint32_t          num_hints,
                                   int32_t           gamma2)
{
    for (uint32_t i = 0U; i < MLDSA_N; i++) {
        uint32_t in_list = 0U;
        for (uint32_t hidx = 0U; hidx < num_hints; hidx++) {
            if (i == h_indices[hidx]) {
                in_list = 1U;
                break;
            }
        }
        b->coeffs[i] = MLDSA_ROUNDING_use_hint(a->coeffs[i], in_list, gamma2);
    }
}

/*********************
 *  STREAMING GAMMA1
 *********************/

void MLDSA_LOWRAM_sample_gamma1(mldsa_poly   *a,
                                const uint8_t seed[MLDSA_CRHBYTES],
                                uint16_t      nonce,
                                int32_t       gamma1)
{
    MLDSA_SAMPLE_gamma1(a, seed, nonce, gamma1);
}

void MLDSA_LOWRAM_sample_gamma1_add(mldsa_poly       *a,
                                    const mldsa_poly *b,
                                    const uint8_t     seed[MLDSA_CRHBYTES],
                                    uint16_t          nonce,
                                    int32_t           gamma1)
{
    // gamma1 = 2^17 => 18 bits/coeff => 576 bytes.
    // gamma1 = 2^19 => 20 bits/coeff => 640 bytes.
    uint8_t buf[640U];
    size_t  buflen = (gamma1 == (1 << 17)) ? 576U : 640U;

    MLDSA_UTIL_shake256_seed_nonce(buf, buflen, seed, MLDSA_CRHBYTES, nonce);

    if (gamma1 == (1 << 17)) {
        for (uint32_t i = 0U; i < MLDSA_N / 4U; i++) {
            uint32_t t0, t1, t2, t3;
            t0 = (uint32_t) buf[9U * i + 0U] | ((uint32_t) buf[9U * i + 1U] << 8U)
                 | ((uint32_t) buf[9U * i + 2U] << 16U);
            t0 &= 0x3FFFFU;
            t1 = ((uint32_t) buf[9U * i + 2U] >> 2U) | ((uint32_t) buf[9U * i + 3U] << 6U)
                 | ((uint32_t) buf[9U * i + 4U] << 14U);
            t1 &= 0x3FFFFU;
            t2 = ((uint32_t) buf[9U * i + 4U] >> 4U) | ((uint32_t) buf[9U * i + 5U] << 4U)
                 | ((uint32_t) buf[9U * i + 6U] << 12U);
            t2 &= 0x3FFFFU;
            t3 = ((uint32_t) buf[9U * i + 6U] >> 6U) | ((uint32_t) buf[9U * i + 7U] << 2U)
                 | ((uint32_t) buf[9U * i + 8U] << 10U);
            t3 &= 0x3FFFFU;

            a->coeffs[4U * i + 0U] = b->coeffs[4U * i + 0U] + gamma1 - (int32_t) t0;
            a->coeffs[4U * i + 1U] = b->coeffs[4U * i + 1U] + gamma1 - (int32_t) t1;
            a->coeffs[4U * i + 2U] = b->coeffs[4U * i + 2U] + gamma1 - (int32_t) t2;
            a->coeffs[4U * i + 3U] = b->coeffs[4U * i + 3U] + gamma1 - (int32_t) t3;
        }
    }
    else {
        for (uint32_t i = 0U; i < MLDSA_N / 2U; i++) {
            uint32_t t0, t1;
            t0 = (uint32_t) buf[5U * i + 0U] | ((uint32_t) buf[5U * i + 1U] << 8U)
                 | ((uint32_t) buf[5U * i + 2U] << 16U);
            t0 &= 0xFFFFFU;
            t1 = ((uint32_t) buf[5U * i + 2U] >> 4U) | ((uint32_t) buf[5U * i + 3U] << 4U)
                 | ((uint32_t) buf[5U * i + 4U] << 12U);
            t1 &= 0xFFFFFU;

            a->coeffs[2U * i + 0U] = b->coeffs[2U * i + 0U] + gamma1 - (int32_t) t0;
            a->coeffs[2U * i + 1U] = b->coeffs[2U * i + 1U] + gamma1 - (int32_t) t1;
        }
    }

    explicit_bzero(buf, sizeof(buf));
}

#endif /* HAVE_MLDSA_OPTIMIZATION */
