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
 * @file    cx_mlkem_params.c
 * @brief   ML-KEM parameter table definition.
 */

#include "lcx_mlkem.h"

const MLKEM_param_info_t MLKEM_PARAM[MLKEM_NUM_PARAM_SETS] = {
  /* ML-KEM-512 */
    {.k                        = 2,
     .eta1                     = 3,
     .eta2                     = 2,
     .du                       = 10,
     .dv                       = 4,
     .polyvec_bytes            = MLKEM512_POLYVECBYTES,
     .polyvec_compressed_bytes = MLKEM512_POLYVECCOMPRESSEDBYTES,
     .poly_compressed_dv_bytes = MLKEM512_POLYCOMPRESSEDBYTES_DV,
     .indcpa_pk_bytes          = MLKEM512_INDCPA_PUBLICKEYBYTES,
     .indcpa_sk_bytes          = MLKEM512_INDCPA_SECRETKEYBYTES,
     .indcpa_ct_bytes          = MLKEM512_INDCPA_BYTES,
     .pk_bytes                 = MLKEM512_PUBLICKEYBYTES,
     .sk_bytes                 = MLKEM512_SECRETKEYBYTES,
     .ct_bytes                 = MLKEM512_CIPHERTEXTBYTES },
 /* ML-KEM-768 */
    {.k                        = 3,
     .eta1                     = 2,
     .eta2                     = 2,
     .du                       = 10,
     .dv                       = 4,
     .polyvec_bytes            = MLKEM768_POLYVECBYTES,
     .polyvec_compressed_bytes = MLKEM768_POLYVECCOMPRESSEDBYTES,
     .poly_compressed_dv_bytes = MLKEM768_POLYCOMPRESSEDBYTES_DV,
     .indcpa_pk_bytes          = MLKEM768_INDCPA_PUBLICKEYBYTES,
     .indcpa_sk_bytes          = MLKEM768_INDCPA_SECRETKEYBYTES,
     .indcpa_ct_bytes          = MLKEM768_INDCPA_BYTES,
     .pk_bytes                 = MLKEM768_PUBLICKEYBYTES,
     .sk_bytes                 = MLKEM768_SECRETKEYBYTES,
     .ct_bytes                 = MLKEM768_CIPHERTEXTBYTES },
 /* ML-KEM-1024 */
    {.k                        = 4,
     .eta1                     = 2,
     .eta2                     = 2,
     .du                       = 11,
     .dv                       = 5,
     .polyvec_bytes            = MLKEM1024_POLYVECBYTES,
     .polyvec_compressed_bytes = MLKEM1024_POLYVECCOMPRESSEDBYTES,
     .poly_compressed_dv_bytes = MLKEM1024_POLYCOMPRESSEDBYTES_DV,
     .indcpa_pk_bytes          = MLKEM1024_INDCPA_PUBLICKEYBYTES,
     .indcpa_sk_bytes          = MLKEM1024_INDCPA_SECRETKEYBYTES,
     .indcpa_ct_bytes          = MLKEM1024_INDCPA_BYTES,
     .pk_bytes                 = MLKEM1024_PUBLICKEYBYTES,
     .sk_bytes                 = MLKEM1024_SECRETKEYBYTES,
     .ct_bytes                 = MLKEM1024_CIPHERTEXTBYTES}
};
