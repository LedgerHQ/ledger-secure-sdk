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
#ifndef CX_MLDSA_INTERNAL_H
#define CX_MLDSA_INTERNAL_H

#include "lcx_mldsa.h"

typedef struct MLDSA_formatted_message_s MLDSA_formatted_message_t;

cx_err_t MLDSA_internal_sign_core(uint8_t                         *sig,
                                  size_t                           sig_len,
                                  size_t                          *sig_actual_len,
                                  const MLDSA_formatted_message_t *formatted_mprime,
                                  const uint8_t                   *precomputed_mu,
                                  uint8_t                         *rnd,
                                  size_t                           rnd_len,
                                  const uint8_t                   *sk,
                                  size_t                           sk_len,
                                  MLDSA_param_t                    param);

cx_err_t MLDSA_internal_verify_core(const uint8_t                   *sig,
                                    size_t                           sig_len,
                                    const MLDSA_formatted_message_t *formatted_mprime,
                                    const uint8_t                   *precomputed_mu,
                                    const uint8_t                   *pk,
                                    size_t                           pk_len,
                                    MLDSA_param_t                    param);

/**
 * @brief   Generates an ML-DSA key pair from a seed (deterministic).
 *
 * @param[out] pk      Public key output buffer.
 * @param[in]  pk_len  Size of the public key buffer in bytes.
 * @param[out] sk      Secret key output buffer.
 * @param[in]  sk_len  Size of the secret key buffer in bytes.
 * @param[in]  seed    Random seed of MLDSA_SEEDBYTES bytes.
 * @param[in]  param   ML-DSA parameter set selector.
 *
 * @return             CX_OK on success, error code otherwise.
 */
cx_err_t MLDSA_internal_keygen(uint8_t      *pk,
                               size_t        pk_len,
                               uint8_t      *sk,
                               size_t        sk_len,
                               const uint8_t seed[MLDSA_SEEDBYTES],
                               MLDSA_param_t param);

/**
 * @brief   Internal ML-DSA signature generation (FIPS 204, Algorithms 2 & 7).
 *
 * Signs a message whose mu value has already been computed by the caller.
 * Unlike MLDSA_sign(), this function does not perform message formatting
 * or context string handling.
 *
 * @param[out] sig             Signature output buffer.
 * @param[in]  sig_len         Size of the signature buffer in bytes.
 * @param[out] sig_actual_len  Actual signature length written.
 * @param[in]  mu              Pre-computed message representative (MLDSA_CRHBYTES bytes).
 * @param[in]  rnd             Signing randomness buffer (MLDSA_RNDBYTES bytes).
 * @param[in]  rnd_len         Length of the randomness buffer in bytes.
 * @param[in]  sk              Secret key.
 * @param[in]  sk_len          Secret key length in bytes.
 * @param[in]  param           ML-DSA parameter set selector.
 *
 * @return                     CX_OK on success, error code otherwise.
 */
cx_err_t MLDSA_internal_sign(uint8_t       *sig,
                             size_t         sig_len,
                             size_t        *sig_actual_len,
                             const uint8_t *mu,
                             uint8_t       *rnd,
                             size_t         rnd_len,
                             const uint8_t *sk,
                             size_t         sk_len,
                             MLDSA_param_t  param);

/**
 * @brief   Internal ML-DSA signature verification (FIPS 204, Algorithms 3 & 8).
 *
 * Verifies a signature given a pre-computed message representative mu.
 * Unlike MLDSA_verify(), this function does not perform message formatting
 * or context string handling.
 *
 * @param[in]  sig      Signature to verify.
 * @param[in]  sig_len  Signature length in bytes.
 * @param[in]  mu       Pre-computed message representative (MLDSA_CRHBYTES bytes).
 * @param[in]  pk       Public key.
 * @param[in]  pk_len   Public key length in bytes.
 * @param[in]  param    ML-DSA parameter set selector.
 *
 * @return              CX_OK if signature is valid, error code otherwise.
 */
cx_err_t MLDSA_internal_verify(const uint8_t *sig,
                               size_t         sig_len,
                               const uint8_t *mu,
                               const uint8_t *pk,
                               size_t         pk_len,
                               MLDSA_param_t  param);

#endif /* CX_MLDSA_INTERNAL_H */
