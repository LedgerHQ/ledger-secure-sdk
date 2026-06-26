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

#ifndef CX_MLKEM_INDCPA_H
#define CX_MLKEM_INDCPA_H

#include <stdint.h>
#include "cx_errors.h"
#include "lcx_mlkem.h"

/**
 * @brief   Generates an IND-CPA key pair deterministically.
 *
 * @param[out] pk      Public key output buffer.
 * @param[in]  pk_len  Size of the public key buffer in bytes.
 * @param[out] sk      Secret key output buffer.
 * @param[in]  sk_len  Size of the secret key buffer in bytes.
 * @param[in]  coins   Random coins of 2 * MLKEM_SYMBYTES bytes.
 * @param[in]  p       Pointer to the ML-KEM parameter set.
 *
 * @return             CX_OK on success, error code otherwise.
 */
cx_err_t MLKEM_INDCPA_keypair_derand(uint8_t                  *pk,
                                     size_t                    pk_len,
                                     uint8_t                  *sk,
                                     size_t                    sk_len,
                                     const uint8_t            *coins,
                                     const MLKEM_param_info_t *p);

cx_err_t MLKEM_INDCPA_enc(uint8_t                  *c,
                          size_t                    c_len,
                          const uint8_t            *m,
                          size_t                    m_len,
                          const uint8_t            *pk,
                          size_t                    pk_len,
                          const uint8_t             coins[2 * MLKEM_SYMBYTES],
                          const MLKEM_param_info_t *p);

cx_err_t MLKEM_INDCPA_dec(uint8_t                  *m,
                          size_t                    m_len,
                          const uint8_t            *c,
                          size_t                    c_len,
                          const uint8_t            *sk,
                          size_t                    sk_len,
                          const MLKEM_param_info_t *p);

#endif  // CX_MLKEM_INDCPA_H
