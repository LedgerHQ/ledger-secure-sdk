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
#ifndef CX_MLKEM_UTIL_H
#define CX_MLKEM_UTIL_H

#include <stdint.h>
#include "lcx_sha3.h"

/**
 * @brief   Computes the H hash function (SHA3-256) as defined in FIPS 203.
 *
 * @param[out] out     Output buffer of size CX_SHA3_256_SIZE bytes.
 * @param[in]  in      Input data to hash.
 * @param[in]  in_len  Length of the input data in bytes.
 */
void MLKEM_UTIL_hash_h(uint8_t out[CX_SHA3_256_SIZE], const uint8_t *in, size_t in_len);

/**
 * @brief   Computes the G hash function (SHA3-512) as defined in FIPS 203.
 *
 * @param[out] out     Output buffer of at least 64 bytes.
 * @param[in]  in      Input data to hash.
 * @param[in]  in_len  Length of the input data in bytes.
 */
void MLKEM_UTIL_hash_g(uint8_t *out, const uint8_t *in, size_t in_len);

void MLKEM_UTIL_hash_j(uint8_t *out, const uint8_t *in, size_t inlen);

/**
 * @brief   Computes the PRF function (SHAKE256) as defined in FIPS 203.
 *
 * @param[out] out     Output buffer.
 * @param[in]  outlen  Number of bytes to produce.
 * @param[in]  key     Input key of MLKEM_SYMBYTES bytes.
 * @param[in]  nonce   Single-byte nonce appended to the key.
 */
void MLKEM_UTIL_prf(uint8_t *out, size_t outlen, const uint8_t *key, uint8_t nonce);

/**
 * @brief   Computes the XOF function (SHAKE256) as defined in FIPS 203.
 *
 * @param[out] out     Output buffer.
 * @param[in]  outlen  Number of bytes to produce.
 * @param[in]  seed    Input seed of MLKEM_SYMBYTES bytes.
 * @param[in]  x       First index byte appended to the seed.
 * @param[in]  y       Second index byte appended to the seed.
 */
void MLKEM_UTIL_xof_squeeze(uint8_t *out, size_t outlen, const uint8_t *seed, uint8_t x, uint8_t y);

void MLKEM_UTIL_ct_cmov(uint8_t *dst, const uint8_t *src, size_t len, uint8_t b);

/**
 * @brief   Validates encapsulation key modulus (FIPS 203 §7.2).
 *
 * Verifies that every 12-bit decoded coefficient of the public polynomial vector is in [0, q-1].
 *
 * @param[in] ek  Encapsulation key bytes (polynomial vector portion).
 * @param[in] p   ML-KEM parameter set.
 *
 * @return        cx_err_t
 * @retval CX_OK: ek is valid
 * @retval CX_INVALID_PARAMETER: ek is not valid.
 */
cx_err_t MLKEM_UTIL_check_ek(const uint8_t *ek, const MLKEM_param_info_t *p);

#endif  // CX_MLKEM_UTIL_H
