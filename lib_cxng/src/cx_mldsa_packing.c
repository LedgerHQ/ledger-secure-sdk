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
 * @file    cx_mldsa_packing.c
 * @brief   ML-DSA packing/unpacking operations (FIPS 204).
 */

#include <string.h>
#include "cx_mldsa_packing.h"

/*---------------------------------------------------------------------------
 * t1 packing: 10 bits per coefficient, 256 coefficients = 320 bytes.
 *---------------------------------------------------------------------------*/
void MLDSA_PACK_polyt1(uint8_t r[MLDSA_POLYT1_PACKEDBYTES], const mldsa_poly *a)
{
    for (uint32_t i = 0U; i < MLDSA_N / 4U; i++) {
        r[5U * i + 0U] = (uint8_t) (a->coeffs[4U * i + 0U] >> 0U);
        r[5U * i + 1U]
            = (uint8_t) ((a->coeffs[4U * i + 0U] >> 8U) | (a->coeffs[4U * i + 1U] << 2U));
        r[5U * i + 2U]
            = (uint8_t) ((a->coeffs[4U * i + 1U] >> 6U) | (a->coeffs[4U * i + 2U] << 4U));
        r[5U * i + 3U]
            = (uint8_t) ((a->coeffs[4U * i + 2U] >> 4U) | (a->coeffs[4U * i + 3U] << 6U));
        r[5U * i + 4U] = (uint8_t) (a->coeffs[4U * i + 3U] >> 2U);
    }
}

void MLDSA_PACK_unpack_polyt1(mldsa_poly *r, const uint8_t a[MLDSA_POLYT1_PACKEDBYTES])
{
    for (uint32_t i = 0U; i < MLDSA_N / 4U; i++) {
        r->coeffs[4U * i + 0U]
            = (((uint32_t) a[5U * i + 0U] >> 0U) | ((uint32_t) a[5U * i + 1U] << 8U)) & 0x3FFU;
        r->coeffs[4U * i + 1U]
            = (((uint32_t) a[5U * i + 1U] >> 2U) | ((uint32_t) a[5U * i + 2U] << 6U)) & 0x3FFU;
        r->coeffs[4U * i + 2U]
            = (((uint32_t) a[5U * i + 2U] >> 4U) | ((uint32_t) a[5U * i + 3U] << 4U)) & 0x3FFU;
        r->coeffs[4U * i + 3U]
            = (((uint32_t) a[5U * i + 3U] >> 6U) | ((uint32_t) a[5U * i + 4U] << 2U)) & 0x3FFU;
    }
}

/*---------------------------------------------------------------------------
 * t0 packing: 13 bits per coefficient, coefficients in (-(2^12), 2^12].
 *---------------------------------------------------------------------------*/
void MLDSA_PACK_polyt0(uint8_t r[MLDSA_POLYT0_PACKEDBYTES], const mldsa_poly *a)
{
    int32_t t[8];
    for (uint32_t i = 0U; i < MLDSA_N / 8U; i++) {
        t[0] = (1 << (MLDSA_D - 1)) - a->coeffs[8U * i + 0U];
        t[1] = (1 << (MLDSA_D - 1)) - a->coeffs[8U * i + 1U];
        t[2] = (1 << (MLDSA_D - 1)) - a->coeffs[8U * i + 2U];
        t[3] = (1 << (MLDSA_D - 1)) - a->coeffs[8U * i + 3U];
        t[4] = (1 << (MLDSA_D - 1)) - a->coeffs[8U * i + 4U];
        t[5] = (1 << (MLDSA_D - 1)) - a->coeffs[8U * i + 5U];
        t[6] = (1 << (MLDSA_D - 1)) - a->coeffs[8U * i + 6U];
        t[7] = (1 << (MLDSA_D - 1)) - a->coeffs[8U * i + 7U];

        r[13U * i + 0U] = (uint8_t) t[0];
        r[13U * i + 1U] = (uint8_t) (t[0] >> 8U);
        r[13U * i + 1U] |= (uint8_t) (t[1] << 5U);
        r[13U * i + 2U] = (uint8_t) (t[1] >> 3U);
        r[13U * i + 3U] = (uint8_t) (t[1] >> 11U);
        r[13U * i + 3U] |= (uint8_t) (t[2] << 2U);
        r[13U * i + 4U] = (uint8_t) (t[2] >> 6U);
        r[13U * i + 4U] |= (uint8_t) (t[3] << 7U);
        r[13U * i + 5U] = (uint8_t) (t[3] >> 1U);
        r[13U * i + 6U] = (uint8_t) (t[3] >> 9U);
        r[13U * i + 6U] |= (uint8_t) (t[4] << 4U);
        r[13U * i + 7U] = (uint8_t) (t[4] >> 4U);
        r[13U * i + 8U] = (uint8_t) (t[4] >> 12U);
        r[13U * i + 8U] |= (uint8_t) (t[5] << 1U);
        r[13U * i + 9U] = (uint8_t) (t[5] >> 7U);
        r[13U * i + 9U] |= (uint8_t) (t[6] << 6U);
        r[13U * i + 10U] = (uint8_t) (t[6] >> 2U);
        r[13U * i + 11U] = (uint8_t) (t[6] >> 10U);
        r[13U * i + 11U] |= (uint8_t) (t[7] << 3U);
        r[13U * i + 12U] = (uint8_t) (t[7] >> 5U);
    }
}

void MLDSA_PACK_unpack_polyt0(mldsa_poly *r, const uint8_t a[MLDSA_POLYT0_PACKEDBYTES])
{
    for (uint32_t i = 0U; i < MLDSA_N / 8U; i++) {
        r->coeffs[8U * i + 0U]
            = (int32_t) (((uint32_t) a[13U * i + 0U] | ((uint32_t) a[13U * i + 1U] << 8U))
                         & 0x1FFFU);
        r->coeffs[8U * i + 1U]
            = (int32_t) ((((uint32_t) a[13U * i + 1U] >> 5U) | ((uint32_t) a[13U * i + 2U] << 3U)
                          | ((uint32_t) a[13U * i + 3U] << 11U))
                         & 0x1FFFU);
        r->coeffs[8U * i + 2U]
            = (int32_t) ((((uint32_t) a[13U * i + 3U] >> 2U) | ((uint32_t) a[13U * i + 4U] << 6U))
                         & 0x1FFFU);
        r->coeffs[8U * i + 3U]
            = (int32_t) ((((uint32_t) a[13U * i + 4U] >> 7U) | ((uint32_t) a[13U * i + 5U] << 1U)
                          | ((uint32_t) a[13U * i + 6U] << 9U))
                         & 0x1FFFU);
        r->coeffs[8U * i + 4U]
            = (int32_t) ((((uint32_t) a[13U * i + 6U] >> 4U) | ((uint32_t) a[13U * i + 7U] << 4U)
                          | ((uint32_t) a[13U * i + 8U] << 12U))
                         & 0x1FFFU);
        r->coeffs[8U * i + 5U]
            = (int32_t) ((((uint32_t) a[13U * i + 8U] >> 1U) | ((uint32_t) a[13U * i + 9U] << 7U))
                         & 0x1FFFU);
        r->coeffs[8U * i + 6U]
            = (int32_t) ((((uint32_t) a[13U * i + 9U] >> 6U) | ((uint32_t) a[13U * i + 10U] << 2U)
                          | ((uint32_t) a[13U * i + 11U] << 10U))
                         & 0x1FFFU);
        r->coeffs[8U * i + 7U]
            = (int32_t) ((((uint32_t) a[13U * i + 11U] >> 3U) | ((uint32_t) a[13U * i + 12U] << 5U))
                         & 0x1FFFU);

        r->coeffs[8U * i + 0U] = (1 << (MLDSA_D - 1)) - r->coeffs[8U * i + 0U];
        r->coeffs[8U * i + 1U] = (1 << (MLDSA_D - 1)) - r->coeffs[8U * i + 1U];
        r->coeffs[8U * i + 2U] = (1 << (MLDSA_D - 1)) - r->coeffs[8U * i + 2U];
        r->coeffs[8U * i + 3U] = (1 << (MLDSA_D - 1)) - r->coeffs[8U * i + 3U];
        r->coeffs[8U * i + 4U] = (1 << (MLDSA_D - 1)) - r->coeffs[8U * i + 4U];
        r->coeffs[8U * i + 5U] = (1 << (MLDSA_D - 1)) - r->coeffs[8U * i + 5U];
        r->coeffs[8U * i + 6U] = (1 << (MLDSA_D - 1)) - r->coeffs[8U * i + 6U];
        r->coeffs[8U * i + 7U] = (1 << (MLDSA_D - 1)) - r->coeffs[8U * i + 7U];
    }
}

/*---------------------------------------------------------------------------
 * eta packing
 *---------------------------------------------------------------------------*/
uint32_t MLDSA_PACK_polyeta(uint8_t *r, const mldsa_poly *a, uint8_t eta)
{
    if (eta == 2U) {
        // 3 bits per coefficient, pack 8 coefficients in 3 bytes
        for (uint32_t i = 0U; i < MLDSA_N / 8U; i++) {
            uint8_t t[8];
            for (uint32_t j = 0U; j < 8U; j++) {
                t[j] = (uint8_t) ((int32_t) eta - a->coeffs[8U * i + j]);
            }

            r[3U * i + 0U] = (t[0] >> 0U) | (t[1] << 3U) | (t[2] << 6U);
            r[3U * i + 1U] = (t[2] >> 2U) | (t[3] << 1U) | (t[4] << 4U) | (t[5] << 7U);
            r[3U * i + 2U] = (t[5] >> 1U) | (t[6] << 2U) | (t[7] << 5U);
        }
        return MLDSA_N * 3U / 8U;  // 96 bytes
    }
    else {
        // eta == 4: 4 bits per coefficient
        for (uint32_t i = 0U; i < MLDSA_N / 2U; i++) {
            uint8_t t0 = (uint8_t) ((int32_t) eta - a->coeffs[2U * i + 0U]);
            uint8_t t1 = (uint8_t) ((int32_t) eta - a->coeffs[2U * i + 1U]);
            r[i]       = t0 | (t1 << 4U);
        }
        return MLDSA_N / 2U;  // 128 bytes
    }
}

uint32_t MLDSA_PACK_unpack_polyeta(mldsa_poly *r, const uint8_t *a, uint8_t eta)
{
    if (eta == 2U) {
        for (uint32_t i = 0U; i < MLDSA_N / 8U; i++) {
            r->coeffs[8U * i + 0U] = (int32_t) ((a[3U * i + 0U] >> 0U) & 7U);
            r->coeffs[8U * i + 1U] = (int32_t) ((a[3U * i + 0U] >> 3U) & 7U);
            r->coeffs[8U * i + 2U]
                = (int32_t) (((a[3U * i + 0U] >> 6U) | (a[3U * i + 1U] << 2U)) & 7U);
            r->coeffs[8U * i + 3U] = (int32_t) ((a[3U * i + 1U] >> 1U) & 7U);
            r->coeffs[8U * i + 4U] = (int32_t) ((a[3U * i + 1U] >> 4U) & 7U);
            r->coeffs[8U * i + 5U]
                = (int32_t) (((a[3U * i + 1U] >> 7U) | (a[3U * i + 2U] << 1U)) & 7U);
            r->coeffs[8U * i + 6U] = (int32_t) ((a[3U * i + 2U] >> 2U) & 7U);
            r->coeffs[8U * i + 7U] = (int32_t) ((a[3U * i + 2U] >> 5U) & 7U);

            r->coeffs[8U * i + 0U] = (int32_t) eta - r->coeffs[8U * i + 0U];
            r->coeffs[8U * i + 1U] = (int32_t) eta - r->coeffs[8U * i + 1U];
            r->coeffs[8U * i + 2U] = (int32_t) eta - r->coeffs[8U * i + 2U];
            r->coeffs[8U * i + 3U] = (int32_t) eta - r->coeffs[8U * i + 3U];
            r->coeffs[8U * i + 4U] = (int32_t) eta - r->coeffs[8U * i + 4U];
            r->coeffs[8U * i + 5U] = (int32_t) eta - r->coeffs[8U * i + 5U];
            r->coeffs[8U * i + 6U] = (int32_t) eta - r->coeffs[8U * i + 6U];
            r->coeffs[8U * i + 7U] = (int32_t) eta - r->coeffs[8U * i + 7U];
        }
        return MLDSA_N * 3U / 8U;
    }
    else {
        for (uint32_t i = 0U; i < MLDSA_N / 2U; i++) {
            r->coeffs[2U * i + 0U] = (int32_t) (a[i] & 0x0FU);
            r->coeffs[2U * i + 1U] = (int32_t) (a[i] >> 4U);
            r->coeffs[2U * i + 0U] = (int32_t) eta - r->coeffs[2U * i + 0U];
            r->coeffs[2U * i + 1U] = (int32_t) eta - r->coeffs[2U * i + 1U];
        }
        return MLDSA_N / 2U;
    }
}

/*---------------------------------------------------------------------------
 * z packing
 *---------------------------------------------------------------------------*/
uint32_t MLDSA_PACK_polyz(uint8_t *r, const mldsa_poly *a, int32_t gamma1)
{
    int32_t t[4];

    if (gamma1 == (1 << 17)) {
        // 18 bits per coefficient
        for (uint32_t i = 0U; i < MLDSA_N / 4U; i++) {
            t[0] = gamma1 - a->coeffs[4U * i + 0U];
            t[1] = gamma1 - a->coeffs[4U * i + 1U];
            t[2] = gamma1 - a->coeffs[4U * i + 2U];
            t[3] = gamma1 - a->coeffs[4U * i + 3U];

            r[9U * i + 0U] = (uint8_t) t[0];
            r[9U * i + 1U] = (uint8_t) (t[0] >> 8U);
            r[9U * i + 2U] = (uint8_t) ((t[0] >> 16U) | (t[1] << 2U));
            r[9U * i + 3U] = (uint8_t) (t[1] >> 6U);
            r[9U * i + 4U] = (uint8_t) ((t[1] >> 14U) | (t[2] << 4U));
            r[9U * i + 5U] = (uint8_t) (t[2] >> 4U);
            r[9U * i + 6U] = (uint8_t) ((t[2] >> 12U) | (t[3] << 6U));
            r[9U * i + 7U] = (uint8_t) (t[3] >> 2U);
            r[9U * i + 8U] = (uint8_t) (t[3] >> 10U);
        }
        return 576U;
    }
    else {
        // 20 bits per coefficient
        for (uint32_t i = 0U; i < MLDSA_N / 2U; i++) {
            t[0] = gamma1 - a->coeffs[2U * i + 0U];
            t[1] = gamma1 - a->coeffs[2U * i + 1U];

            r[5U * i + 0U] = (uint8_t) t[0];
            r[5U * i + 1U] = (uint8_t) (t[0] >> 8U);
            r[5U * i + 2U] = (uint8_t) ((t[0] >> 16U) | (t[1] << 4U));
            r[5U * i + 3U] = (uint8_t) (t[1] >> 4U);
            r[5U * i + 4U] = (uint8_t) (t[1] >> 12U);
        }
        return 640U;
    }
}

uint32_t MLDSA_PACK_unpack_polyz(mldsa_poly *r, const uint8_t *a, int32_t gamma1)
{
    if (gamma1 == (1 << 17)) {
        for (uint32_t i = 0U; i < MLDSA_N / 4U; i++) {
            r->coeffs[4U * i + 0U]
                = (int32_t) (((uint32_t) a[9U * i + 0U] | ((uint32_t) a[9U * i + 1U] << 8U)
                              | ((uint32_t) a[9U * i + 2U] << 16U))
                             & 0x3FFFFU);
            r->coeffs[4U * i + 1U]
                = (int32_t) ((((uint32_t) a[9U * i + 2U] >> 2U) | ((uint32_t) a[9U * i + 3U] << 6U)
                              | ((uint32_t) a[9U * i + 4U] << 14U))
                             & 0x3FFFFU);
            r->coeffs[4U * i + 2U]
                = (int32_t) ((((uint32_t) a[9U * i + 4U] >> 4U) | ((uint32_t) a[9U * i + 5U] << 4U)
                              | ((uint32_t) a[9U * i + 6U] << 12U))
                             & 0x3FFFFU);
            r->coeffs[4U * i + 3U]
                = (int32_t) ((((uint32_t) a[9U * i + 6U] >> 6U) | ((uint32_t) a[9U * i + 7U] << 2U)
                              | ((uint32_t) a[9U * i + 8U] << 10U))
                             & 0x3FFFFU);

            r->coeffs[4U * i + 0U] = gamma1 - r->coeffs[4U * i + 0U];
            r->coeffs[4U * i + 1U] = gamma1 - r->coeffs[4U * i + 1U];
            r->coeffs[4U * i + 2U] = gamma1 - r->coeffs[4U * i + 2U];
            r->coeffs[4U * i + 3U] = gamma1 - r->coeffs[4U * i + 3U];
        }
        return 576U;
    }
    else {
        for (uint32_t i = 0U; i < MLDSA_N / 2U; i++) {
            r->coeffs[2U * i + 0U]
                = (int32_t) (((uint32_t) a[5U * i + 0U] | ((uint32_t) a[5U * i + 1U] << 8U)
                              | ((uint32_t) a[5U * i + 2U] << 16U))
                             & 0xFFFFFU);
            r->coeffs[2U * i + 1U]
                = (int32_t) ((((uint32_t) a[5U * i + 2U] >> 4U) | ((uint32_t) a[5U * i + 3U] << 4U)
                              | ((uint32_t) a[5U * i + 4U] << 12U))
                             & 0xFFFFFU);

            r->coeffs[2U * i + 0U] = gamma1 - r->coeffs[2U * i + 0U];
            r->coeffs[2U * i + 1U] = gamma1 - r->coeffs[2U * i + 1U];
        }
        return 640U;
    }
}

/*---------------------------------------------------------------------------
 * w1 packing
 *---------------------------------------------------------------------------*/
uint32_t MLDSA_PACK_polyw1(uint8_t *r, const mldsa_poly *a, int32_t gamma2)
{
    if (gamma2 == (MLDSA_Q - 1) / 88) {
        // 6 bits per coefficient (values in [0, 43])
        for (uint32_t i = 0U; i < MLDSA_N / 4U; i++) {
            r[3U * i + 0U] = (uint8_t) (a->coeffs[4U * i + 0U] | (a->coeffs[4U * i + 1U] << 6U));
            r[3U * i + 1U]
                = (uint8_t) ((a->coeffs[4U * i + 1U] >> 2U) | (a->coeffs[4U * i + 2U] << 4U));
            r[3U * i + 2U]
                = (uint8_t) ((a->coeffs[4U * i + 2U] >> 4U) | (a->coeffs[4U * i + 3U] << 2U));
        }
        return 192U;
    }
    else {
        // gamma2 == (q-1)/32: 4 bits per coefficient (values in [0, 15])
        for (uint32_t i = 0U; i < MLDSA_N / 2U; i++) {
            r[i] = (uint8_t) (a->coeffs[2U * i + 0U] | (a->coeffs[2U * i + 1U] << 4U));
        }
        return 128U;
    }
}

/*---------------------------------------------------------------------------
 * Key packing
 *---------------------------------------------------------------------------*/
void MLDSA_PACK_pk(uint8_t                  *pk,
                   const uint8_t             rho[MLDSA_SEEDBYTES],
                   const mldsa_poly         *t1,
                   const MLDSA_param_info_t *p)
{
    memcpy(pk, rho, MLDSA_SEEDBYTES);
    for (uint32_t i = 0U; i < p->k; i++) {
        MLDSA_PACK_polyt1(pk + MLDSA_SEEDBYTES + i * MLDSA_POLYT1_PACKEDBYTES, &t1[i]);
    }
}

void MLDSA_PACK_unpack_pk(uint8_t                   rho[MLDSA_SEEDBYTES],
                          mldsa_poly               *t1,
                          const uint8_t            *pk,
                          const MLDSA_param_info_t *p)
{
    memcpy(rho, pk, MLDSA_SEEDBYTES);
    for (uint32_t i = 0U; i < p->k; i++) {
        MLDSA_PACK_unpack_polyt1(&t1[i], pk + MLDSA_SEEDBYTES + i * MLDSA_POLYT1_PACKEDBYTES);
    }
}

void MLDSA_PACK_sk(uint8_t                  *sk,
                   const uint8_t             rho[MLDSA_SEEDBYTES],
                   const uint8_t             K[MLDSA_SEEDBYTES],
                   const uint8_t             tr[MLDSA_TRBYTES],
                   const mldsa_poly         *s1,
                   const mldsa_poly         *s2,
                   const mldsa_poly         *t0,
                   const MLDSA_param_info_t *p)
{
    uint32_t offset = 0U;

    memcpy(sk + offset, rho, MLDSA_SEEDBYTES);
    offset += MLDSA_SEEDBYTES;

    memcpy(sk + offset, K, MLDSA_SEEDBYTES);
    offset += MLDSA_SEEDBYTES;

    memcpy(sk + offset, tr, MLDSA_TRBYTES);
    offset += MLDSA_TRBYTES;

    for (uint32_t i = 0U; i < p->l; i++) {
        MLDSA_PACK_polyeta(sk + offset, &s1[i], p->eta);
        offset += p->polyeta_packed_bytes;
    }

    for (uint32_t i = 0U; i < p->k; i++) {
        MLDSA_PACK_polyeta(sk + offset, &s2[i], p->eta);
        offset += p->polyeta_packed_bytes;
    }

    for (uint32_t i = 0U; i < p->k; i++) {
        MLDSA_PACK_polyt0(sk + offset, &t0[i]);
        offset += MLDSA_POLYT0_PACKEDBYTES;
    }
}

void MLDSA_PACK_unpack_sk(uint8_t                   rho[MLDSA_SEEDBYTES],
                          uint8_t                   K[MLDSA_SEEDBYTES],
                          uint8_t                   tr[MLDSA_TRBYTES],
                          mldsa_poly               *s1,
                          mldsa_poly               *s2,
                          mldsa_poly               *t0,
                          const uint8_t            *sk,
                          const MLDSA_param_info_t *p)
{
    uint32_t offset = 0U;

    memcpy(rho, sk + offset, MLDSA_SEEDBYTES);
    offset += MLDSA_SEEDBYTES;

    memcpy(K, sk + offset, MLDSA_SEEDBYTES);
    offset += MLDSA_SEEDBYTES;

    memcpy(tr, sk + offset, MLDSA_TRBYTES);
    offset += MLDSA_TRBYTES;

    for (uint32_t i = 0U; i < p->l; i++) {
        MLDSA_PACK_unpack_polyeta(&s1[i], sk + offset, p->eta);
        offset += p->polyeta_packed_bytes;
    }

    for (uint32_t i = 0U; i < p->k; i++) {
        MLDSA_PACK_unpack_polyeta(&s2[i], sk + offset, p->eta);
        offset += p->polyeta_packed_bytes;
    }

    for (uint32_t i = 0U; i < p->k; i++) {
        MLDSA_PACK_unpack_polyt0(&t0[i], sk + offset);
        offset += MLDSA_POLYT0_PACKEDBYTES;
    }
}

/*---------------------------------------------------------------------------
 * Signature packing
 *---------------------------------------------------------------------------*/
void MLDSA_PACK_sig(uint8_t                  *sig,
                    const uint8_t            *ctilde,
                    const mldsa_poly         *z,
                    const mldsa_poly         *h,
                    const MLDSA_param_info_t *p)
{
    uint32_t offset = 0U;

    // c_tilde
    memcpy(sig, ctilde, p->ctilde_bytes);
    offset += p->ctilde_bytes;

    // z
    for (uint32_t i = 0U; i < p->l; i++) {
        MLDSA_PACK_polyz(sig + offset, &z[i], p->gamma1);
        offset += p->polyz_packed_bytes;
    }

    // h (hint encoding): for each polynomial i in [0, k), encode positions
    // where hint is 1, then pad with zeros and set last byte to total count.
    memset(sig + offset, 0, p->polyvech_packed_bytes);
    uint32_t k_offset = 0U;
    for (uint32_t i = 0U; i < p->k; i++) {
        for (uint32_t j = 0U; j < MLDSA_N; j++) {
            if (h[i].coeffs[j] != 0) {
                sig[offset + k_offset] = (uint8_t) j;
                k_offset++;
            }
        }
        sig[offset + p->omega + i] = (uint8_t) k_offset;
    }
}

int MLDSA_PACK_unpack_sig(uint8_t                  *ctilde,
                          mldsa_poly               *z,
                          mldsa_poly               *h,
                          const uint8_t            *sig,
                          const MLDSA_param_info_t *p)
{
    uint32_t offset = 0U;

    // c_tilde
    memcpy(ctilde, sig, p->ctilde_bytes);
    offset += p->ctilde_bytes;

    // z
    for (uint32_t i = 0U; i < p->l; i++) {
        MLDSA_PACK_unpack_polyz(&z[i], sig + offset, p->gamma1);
        offset += p->polyz_packed_bytes;
    }

    // h (hint)
    uint32_t k_offset = 0U;
    for (uint32_t i = 0U; i < p->k; i++) {
        for (uint32_t j = 0U; j < MLDSA_N; j++) {
            h[i].coeffs[j] = 0;
        }
    }

    for (uint32_t i = 0U; i < p->k; i++) {
        uint32_t limit = (uint32_t) sig[offset + p->omega + i];

        if (limit < k_offset) {
            return 1;  // Invalid hint encoding
        }
        if (limit > p->omega) {
            return 1;
        }

        for (uint32_t j = k_offset; j < limit; j++) {
            // Coefficients must be in increasing order
            if ((j > k_offset) && (sig[offset + j] <= sig[offset + j - 1U])) {
                return 1;
            }
            h[i].coeffs[sig[offset + j]] = 1;
        }

        k_offset = limit;
    }

    // Check remaining bytes are zero
    for (uint32_t j = k_offset; j < p->omega; j++) {
        if (sig[offset + j] != 0U) {
            return 1;
        }
    }

    return 0;
}
