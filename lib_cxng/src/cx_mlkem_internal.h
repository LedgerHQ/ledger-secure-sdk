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
 * @file    cx_mlkem_internal.h
 * @brief   Internal ML-KEM deterministic routines (not part of the public API).
 *
 * These functions are exposed only for internal testing.  They must NOT be
 * declared in lcx_mlkem.h.
 */

#ifndef CX_MLKEM_INTERNAL_H
#define CX_MLKEM_INTERNAL_H

#include <stdint.h>
#include "cx_errors.h"
#include "lcx_mlkem.h"

/**
 * @brief   Deterministic ML-KEM key pair generation.
 *
 * Generates a key pair from caller-supplied random coins.
 *
 * @param[out] pk      Public key output buffer.
 * @param[in]  pk_len  Size of the public key buffer in bytes.
 * @param[out] sk      Secret key output buffer.
 * @param[in]  sk_len  Size of the secret key buffer in bytes.
 * @param[in]  coins   Random coins of 2 * MLKEM_SYMBYTES bytes.
 * @param[in]  param   ML-KEM parameter set selector.
 *
 * @return             CX_OK on success, error code otherwise.
 */
cx_err_t MLKEM_crypto_kem_keypair_derand(uint8_t      *pk,
                                         size_t        pk_len,
                                         uint8_t      *sk,
                                         size_t        sk_len,
                                         const uint8_t coins[2 * MLKEM_SYMBYTES],
                                         MLKEM_param_t param);

/**
 * @brief   Deterministic ML-KEM encapsulation.
 *
 * Produces a ciphertext and a shared secret from a public key using
 * caller-supplied random coins.
 *
 * @param[out] ct      Ciphertext output buffer.
 * @param[in]  ct_len  Size of the ciphertext buffer in bytes.
 * @param[out] ss      Shared secret output buffer.
 * @param[in]  ss_len  Size of the shared secret buffer in bytes.
 * @param[in]  pk      Public key.
 * @param[in]  pk_len  Size of the public key in bytes.
 * @param[in]  coins   Random coins of 2 * MLKEM_SYMBYTES bytes.
 * @param[in]  param   ML-KEM parameter set selector.
 *
 * @return             CX_OK on success, error code otherwise.
 */
cx_err_t MLKEM_crypto_kem_enc_derand(uint8_t       *ct,
                                     size_t         ct_len,
                                     uint8_t       *ss,
                                     size_t         ss_len,
                                     const uint8_t *pk,
                                     size_t         pk_len,
                                     const uint8_t  coins[2 * MLKEM_SYMBYTES],
                                     MLKEM_param_t  param);

#endif  // CX_MLKEM_INTERNAL_H
