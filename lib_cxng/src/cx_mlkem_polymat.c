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
 * @file    cx_mlkem_polymat.c
 * @brief   ML-KEM polynomial matrix operations (FIPS 203).
 *
 * Implements matrix generation from a seed using rejection sampling.
 */

#include "lcx_mlkem.h"
#include "cx_mlkem_polymat.h"
#include "cx_mlkem_sample.h"

void MLKEM_POLYMAT_gen_matrix(polymat *a, const uint8_t *seed, int32_t transposed, uint8_t k)
{
    for (uint32_t i = 0U; i < k; i++) {
        for (uint32_t j = 0U; j < k; j++) {
            if (transposed != 0) {
                MLKEM_SAMPLE_rej_uniform(&a->vec[i].vec[j], seed, (uint8_t) i, (uint8_t) j);
            }
            else {
                MLKEM_SAMPLE_rej_uniform(&a->vec[i].vec[j], seed, (uint8_t) j, (uint8_t) i);
            }
        }
    }
}

void MLKEM_POLYMAT_gen_matrix_row(polyvec       *row,
                                  const uint8_t *seed,
                                  int32_t        transposed,
                                  uint8_t        i,
                                  uint8_t        k)
{
    for (uint32_t j = 0U; j < k; j++) {
        if (transposed != 0) {
            MLKEM_SAMPLE_rej_uniform(&row->vec[j], seed, i, (uint8_t) j);
        }
        else {
            MLKEM_SAMPLE_rej_uniform(&row->vec[j], seed, (uint8_t) j, i);
        }
    }
}
