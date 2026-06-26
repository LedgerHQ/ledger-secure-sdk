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
#ifndef CX_MLDSA_UTIL_H
#define CX_MLDSA_UTIL_H

#include <stdint.h>
#include <stddef.h>
#include "lcx_sha3.h"

/**
 * @brief   SHAKE256 hash wrapper.
 */
void MLDSA_UTIL_shake256(uint8_t *out, size_t outlen, const uint8_t *in, size_t inlen);

/**
 * @brief   SHAKE128 hash wrapper.
 */
void MLDSA_UTIL_shake128(uint8_t *out, size_t outlen, const uint8_t *in, size_t inlen);

/**
 * @brief   SHAKE256 with two inputs concatenated.
 */
void MLDSA_UTIL_shake256_two(uint8_t       *out,
                             size_t         outlen,
                             const uint8_t *in1,
                             size_t         in1len,
                             const uint8_t *in2,
                             size_t         in2len);

/**
 * @brief   SHAKE256 with three inputs concatenated.
 */
void MLDSA_UTIL_shake256_three(uint8_t       *out,
                               size_t         outlen,
                               const uint8_t *in1,
                               size_t         in1len,
                               const uint8_t *in2,
                               size_t         in2len,
                               const uint8_t *in3,
                               size_t         in3len);

/**
 * @brief   SHAKE128 with seed || uint16_t nonce.
 */
void MLDSA_UTIL_shake128_seed_nonce(uint8_t       *out,
                                    size_t         outlen,
                                    const uint8_t *seed,
                                    size_t         seedlen,
                                    uint16_t       nonce);

/**
 * @brief   SHAKE256 with seed || uint16_t nonce.
 */
void MLDSA_UTIL_shake256_seed_nonce(uint8_t       *out,
                                    size_t         outlen,
                                    const uint8_t *seed,
                                    size_t         seedlen,
                                    uint16_t       nonce);

#endif /* CX_MLDSA_UTIL_H */
