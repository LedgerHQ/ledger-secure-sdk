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
 * @file    cx_mldsa_params.c
 * @brief   ML-DSA parameter table definition.
 */

#include "lcx_mldsa.h"

const MLDSA_param_info_t MLDSA_PARAM[MLDSA_NUM_PARAM_SETS] = {
  /* ML-DSA-44 */
    {.k                     = MLDSA44_K,
     .l                     = MLDSA44_L,
     .eta                   = MLDSA44_ETA,
     .tau                   = MLDSA44_TAU,
     .beta                  = MLDSA44_BETA,
     .gamma1                = MLDSA44_GAMMA1,
     .gamma2                = MLDSA44_GAMMA2,
     .omega                 = MLDSA44_OMEGA,
     .ctilde_bytes          = MLDSA44_CTILDEBYTES,
     .polyeta_packed_bytes  = MLDSA44_POLYETA_PACKEDBYTES,
     .polyz_packed_bytes    = MLDSA44_POLYZ_PACKEDBYTES,
     .polyw1_packed_bytes   = MLDSA44_POLYW1_PACKEDBYTES,
     .polyvech_packed_bytes = MLDSA44_POLYVECH_PACKEDBYTES,
     .pk_bytes              = MLDSA44_PUBLICKEYBYTES,
     .sk_bytes              = MLDSA44_SECRETKEYBYTES,
     .sig_bytes             = MLDSA44_SIGBYTES},
 /* ML-DSA-65 */
    {.k                     = MLDSA65_K,
     .l                     = MLDSA65_L,
     .eta                   = MLDSA65_ETA,
     .tau                   = MLDSA65_TAU,
     .beta                  = MLDSA65_BETA,
     .gamma1                = MLDSA65_GAMMA1,
     .gamma2                = MLDSA65_GAMMA2,
     .omega                 = MLDSA65_OMEGA,
     .ctilde_bytes          = MLDSA65_CTILDEBYTES,
     .polyeta_packed_bytes  = MLDSA65_POLYETA_PACKEDBYTES,
     .polyz_packed_bytes    = MLDSA65_POLYZ_PACKEDBYTES,
     .polyw1_packed_bytes   = MLDSA65_POLYW1_PACKEDBYTES,
     .polyvech_packed_bytes = MLDSA65_POLYVECH_PACKEDBYTES,
     .pk_bytes              = MLDSA65_PUBLICKEYBYTES,
     .sk_bytes              = MLDSA65_SECRETKEYBYTES,
     .sig_bytes             = MLDSA65_SIGBYTES},
#ifdef HAVE_MLDSA_87
  /* ML-DSA-87 */
    {.k                     = MLDSA87_K,
     .l                     = MLDSA87_L,
     .eta                   = MLDSA87_ETA,
     .tau                   = MLDSA87_TAU,
     .beta                  = MLDSA87_BETA,
     .gamma1                = MLDSA87_GAMMA1,
     .gamma2                = MLDSA87_GAMMA2,
     .omega                 = MLDSA87_OMEGA,
     .ctilde_bytes          = MLDSA87_CTILDEBYTES,
     .polyeta_packed_bytes  = MLDSA87_POLYETA_PACKEDBYTES,
     .polyz_packed_bytes    = MLDSA87_POLYZ_PACKEDBYTES,
     .polyw1_packed_bytes   = MLDSA87_POLYW1_PACKEDBYTES,
     .polyvech_packed_bytes = MLDSA87_POLYVECH_PACKEDBYTES,
     .pk_bytes              = MLDSA87_PUBLICKEYBYTES,
     .sk_bytes              = MLDSA87_SECRETKEYBYTES,
     .sig_bytes             = MLDSA87_SIGBYTES},
#endif
};
