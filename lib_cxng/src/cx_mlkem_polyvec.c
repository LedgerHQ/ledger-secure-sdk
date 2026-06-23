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
 * @file    cx_mlkem_polyvec.c
 * @brief   ML-KEM polynomial vector operations (FIPS 203).
 *
 * Implements vector-level wrappers over polynomial operations, including
 * serialization, NTT, reduction, compression/decompression, and
 * accumulated base multiplication.
 */

#include "lcx_mlkem.h"
#include "cx_mlkem_polyvec.h"

/* ========================================================================
 * Polyvec operations (parameterized by k)
 * ====================================================================== */

void MLKEM_POLYVEC_tobytes(uint8_t *r, const polyvec *a, uint8_t k)
{
    for (uint32_t i = 0U; i < k; i++) {
        MLKEM_POLY_tobytes(&r[i * MLKEM_POLYBYTES], &a->vec[i]);
    }
}

void MLKEM_POLYVEC_frombytes(polyvec *r, const uint8_t *a, uint8_t k)
{
    for (uint32_t i = 0U; i < k; i++) {
        MLKEM_POLY_frombytes(&r->vec[i], &a[i * MLKEM_POLYBYTES]);
    }
}

void MLKEM_POLYVEC_ntt(polyvec *r, uint8_t k)
{
    for (uint32_t i = 0U; i < k; i++) {
        MLKEM_POLY_ntt(&r->vec[i]);
    }
}

void MLKEM_POLYVEC_invntt_tomont(polyvec *r, uint8_t k)
{
    for (uint32_t i = 0U; i < k; i++) {
        MLKEM_POLY_invntt_tomont(&r->vec[i]);
    }
}

void MLKEM_POLYVEC_reduce(polyvec *r, uint8_t k)
{
    for (uint32_t i = 0U; i < k; i++) {
        MLKEM_POLY_reduce(&r->vec[i]);
    }
}

void MLKEM_POLYVEC_add(polyvec *r, const polyvec *b, uint8_t k)
{
    for (uint32_t i = 0U; i < k; i++) {
        MLKEM_POLY_add(&r->vec[i], &b->vec[i]);
    }
}

void MLKEM_POLYVEC_tomont(polyvec *r, uint8_t k)
{
    for (uint32_t i = 0U; i < k; i++) {
        MLKEM_POLY_tomont(&r->vec[i]);
    }
}

void MLKEM_POLYVEC_compress(uint8_t *r, const polyvec *a, uint8_t k, uint8_t du)
{
    if (du == 10) {
        for (uint32_t i = 0U; i < k; i++) {
            MLKEM_POLY_compress_d10(&r[i * MLKEM_POLYCOMPRESSEDBYTES_D10], &a->vec[i]);
        }
    }
    else {  // du == 11
        for (uint32_t i = 0U; i < k; i++) {
            MLKEM_POLY_compress_d11(&r[i * MLKEM_POLYCOMPRESSEDBYTES_D11], &a->vec[i]);
        }
    }
}

void MLKEM_POLYVEC_decompress(polyvec *r, const uint8_t *a, uint8_t k, uint8_t du)
{
    if (du == 10) {
        for (uint32_t i = 0U; i < k; i++) {
            MLKEM_POLY_decompress_d10(&r->vec[i], &a[i * MLKEM_POLYCOMPRESSEDBYTES_D10]);
        }
    }
    else {  // du == 11
        for (uint32_t i = 0U; i < k; i++) {
            MLKEM_POLY_decompress_d11(&r->vec[i], &a[i * MLKEM_POLYCOMPRESSEDBYTES_D11]);
        }
    }
}

void MLKEM_POLYVEC_basemul_acc_montgomery(poly *r, const polyvec *a, const polyvec *b, uint8_t k)
{
    for (uint32_t i = 0U; i < k; i++) {
        MLKEM_POLY_basemul_acc_montgomery(r, &a->vec[i], &b->vec[i], (i == 0U) ? 1 : 0);
    }
}
