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
 * @file    lcx_mlkem.h
 * @brief   ML-KEM (Module-Lattice Key Encapsulation Mechanism) public API.
 *
 * Provides the public constants, parameter structures, and function prototypes
 * for ML-KEM-512, ML-KEM-768, and ML-KEM-1024 as specified in FIPS 203.
 */

#ifndef MLKEM_H
#define MLKEM_H

#include <stddef.h>
#include <stdint.h>
#include "cx_errors.h"

/** @defgroup mlkem_common Common ML-KEM Parameters (FIPS 203)
 *  @{
 */

#define MLKEM_N         256U /**< Number of coefficients per polynomial.           */
#define MLKEM_Q         3329 /**< Modulus q.                                       */
#define MLKEM_Q_HALF    1665 /**< (q + 1) / 2, used for message encoding.         */
#define MLKEM_SYMBYTES  32U  /**< Size of seeds, hashes, and shared secrets (bytes). */
#define MLKEM_SSBYTES   32U  /**< Size of the shared secret (bytes).               */
#define MLKEM_POLYBYTES 384U /**< Serialized polynomial size: 12 * N / 8 (bytes).  */

#define MLKEM_POLYCOMPRESSEDBYTES_D10 320U /**< Compressed polynomial size for du=10 (bytes). */
#define MLKEM_POLYCOMPRESSEDBYTES_D11 352U /**< Compressed polynomial size for du=11 (bytes). */

#define MLKEM_NUM_PARAM_SETS 3U /**< Number of supported ML-KEM parameter sets.     */

/** @} */

/** @defgroup mlkem512 ML-KEM-512 Parameters (k=2, eta1=3, eta2=2, du=10, dv=4)
 *  @{
 */

#define MLKEM512_K    2U  /**< Module rank.  */
#define MLKEM512_ETA1 3U  /**< Noise parameter eta1. */
#define MLKEM512_ETA2 2U  /**< Noise parameter eta2. */
#define MLKEM512_DU   10U /**< Compression parameter du. */
#define MLKEM512_DV   4U  /**< Compression parameter dv. */

#define MLKEM512_POLYVECBYTES \
    (MLKEM512_K * MLKEM_POLYBYTES)           /**< Polyvec size: 768 bytes.           */
#define MLKEM512_POLYCOMPRESSEDBYTES_DV 128U /**< Compressed poly size (dv): 128 bytes. */
#define MLKEM512_POLYCOMPRESSEDBYTES_DU 320U /**< Compressed poly size (du): 320 bytes. */
#define MLKEM512_POLYVECCOMPRESSEDBYTES \
    (MLKEM512_K * MLKEM512_POLYCOMPRESSEDBYTES_DU) /**< Compressed polyvec: 640 bytes. */

#define MLKEM512_INDCPA_PUBLICKEYBYTES \
    (MLKEM512_POLYVECBYTES + MLKEM_SYMBYTES) /**< IND-CPA public key: 800 bytes.  */
#define MLKEM512_INDCPA_SECRETKEYBYTES                        \
    MLKEM512_POLYVECBYTES /**< IND-CPA secret key: 768 bytes. \
                           */
#define MLKEM512_INDCPA_BYTES        \
    (MLKEM512_POLYVECCOMPRESSEDBYTES \
     + MLKEM512_POLYCOMPRESSEDBYTES_DV) /**< IND-CPA ciphertext: 768 bytes.  */

#define MLKEM512_PUBLICKEYBYTES \
    MLKEM512_INDCPA_PUBLICKEYBYTES /**< ML-KEM-512 public key: 800 bytes.  */
#define MLKEM512_SECRETKEYBYTES                                      \
    (MLKEM512_INDCPA_SECRETKEYBYTES + MLKEM512_INDCPA_PUBLICKEYBYTES \
     + (2U * MLKEM_SYMBYTES))                          /**< ML-KEM-512 secret key: 1632 bytes. */
#define MLKEM512_CIPHERTEXTBYTES MLKEM512_INDCPA_BYTES /**< ML-KEM-512 ciphertext: 768 bytes.  */

/** @} */

/** @defgroup mlkem768 ML-KEM-768 Parameters (k=3, eta1=2, eta2=2, du=10, dv=4)
 *  @{
 */

#define MLKEM768_K    3U  /**< Module rank.  */
#define MLKEM768_ETA1 2U  /**< Noise parameter eta1. */
#define MLKEM768_ETA2 2U  /**< Noise parameter eta2. */
#define MLKEM768_DU   10U /**< Compression parameter du. */
#define MLKEM768_DV   4U  /**< Compression parameter dv. */

#define MLKEM768_POLYVECBYTES \
    (MLKEM768_K * MLKEM_POLYBYTES)           /**< Polyvec size: 1152 bytes.           */
#define MLKEM768_POLYCOMPRESSEDBYTES_DV 128U /**< Compressed poly size (dv): 128 bytes. */
#define MLKEM768_POLYCOMPRESSEDBYTES_DU 320U /**< Compressed poly size (du): 320 bytes. */
#define MLKEM768_POLYVECCOMPRESSEDBYTES \
    (MLKEM768_K * MLKEM768_POLYCOMPRESSEDBYTES_DU) /**< Compressed polyvec: 960 bytes. */

#define MLKEM768_INDCPA_PUBLICKEYBYTES \
    (MLKEM768_POLYVECBYTES + MLKEM_SYMBYTES) /**< IND-CPA public key: 1184 bytes. */
#define MLKEM768_INDCPA_SECRETKEYBYTES                         \
    MLKEM768_POLYVECBYTES /**< IND-CPA secret key: 1152 bytes. \
                           */
#define MLKEM768_INDCPA_BYTES        \
    (MLKEM768_POLYVECCOMPRESSEDBYTES \
     + MLKEM768_POLYCOMPRESSEDBYTES_DV) /**< IND-CPA ciphertext: 1088 bytes. */

#define MLKEM768_PUBLICKEYBYTES \
    MLKEM768_INDCPA_PUBLICKEYBYTES /**< ML-KEM-768 public key: 1184 bytes.  */
#define MLKEM768_SECRETKEYBYTES                                      \
    (MLKEM768_INDCPA_SECRETKEYBYTES + MLKEM768_INDCPA_PUBLICKEYBYTES \
     + (2U * MLKEM_SYMBYTES))                          /**< ML-KEM-768 secret key: 2400 bytes.  */
#define MLKEM768_CIPHERTEXTBYTES MLKEM768_INDCPA_BYTES /**< ML-KEM-768 ciphertext: 1088 bytes.  */

/** @} */

/** @defgroup mlkem1024 ML-KEM-1024 Parameters (k=4, eta1=2, eta2=2, du=11, dv=5)
 *  @{
 */

#define MLKEM1024_K    4U  /**< Module rank.  */
#define MLKEM1024_ETA1 2U  /**< Noise parameter eta1. */
#define MLKEM1024_ETA2 2U  /**< Noise parameter eta2. */
#define MLKEM1024_DU   11U /**< Compression parameter du. */
#define MLKEM1024_DV   5U  /**< Compression parameter dv. */

#define MLKEM1024_POLYVECBYTES \
    (MLKEM1024_K * MLKEM_POLYBYTES)           /**< Polyvec size: 1536 bytes.           */
#define MLKEM1024_POLYCOMPRESSEDBYTES_DV 160U /**< Compressed poly size (dv): 160 bytes. */
#define MLKEM1024_POLYCOMPRESSEDBYTES_DU 352U /**< Compressed poly size (du): 352 bytes. */
#define MLKEM1024_POLYVECCOMPRESSEDBYTES \
    (MLKEM1024_K * MLKEM1024_POLYCOMPRESSEDBYTES_DU) /**< Compressed polyvec: 1408 bytes. */

#define MLKEM1024_INDCPA_PUBLICKEYBYTES \
    (MLKEM1024_POLYVECBYTES + MLKEM_SYMBYTES) /**< IND-CPA public key: 1568 bytes. */
#define MLKEM1024_INDCPA_SECRETKEYBYTES \
    MLKEM1024_POLYVECBYTES /**< IND-CPA secret key: 1536 bytes. */
#define MLKEM1024_INDCPA_BYTES        \
    (MLKEM1024_POLYVECCOMPRESSEDBYTES \
     + MLKEM1024_POLYCOMPRESSEDBYTES_DV) /**< IND-CPA ciphertext: 1568 bytes. */

#define MLKEM1024_PUBLICKEYBYTES \
    MLKEM1024_INDCPA_PUBLICKEYBYTES /**< ML-KEM-1024 public key: 1568 bytes.  */
#define MLKEM1024_SECRETKEYBYTES                                       \
    (MLKEM1024_INDCPA_SECRETKEYBYTES + MLKEM1024_INDCPA_PUBLICKEYBYTES \
     + (2U * MLKEM_SYMBYTES)) /**< ML-KEM-1024 secret key: 3168 bytes.  */
#define MLKEM1024_CIPHERTEXTBYTES \
    MLKEM1024_INDCPA_BYTES /**< ML-KEM-1024 ciphertext: 1568 bytes.  */

/** @} */

#define MLKEM_MAX_K MLKEM1024_K /**< Maximum module rank across all parameter sets. */

/**
 * @brief   ML-KEM parameter set selector.
 */
typedef enum MLKEM_param_e {
    MLKEM_512 = 0, /**< ML-KEM-512 (NIST security level 1).  */
    MLKEM_768,     /**< ML-KEM-768 (NIST security level 3).  */
    MLKEM_1024,    /**< ML-KEM-1024 (NIST security level 5). */
} MLKEM_param_t;

/**
 * @brief   ML-KEM parameter set descriptor holding all derived sizes.
 */
typedef struct MLKEM_param_info_s {
    uint8_t  k;                        /**< Module rank.                               */
    uint8_t  eta1;                     /**< Noise parameter eta1.                      */
    uint8_t  eta2;                     /**< Noise parameter eta2.                      */
    uint8_t  du;                       /**< Compression parameter du.                  */
    uint8_t  dv;                       /**< Compression parameter dv.                  */
    uint16_t polyvec_bytes;            /**< Serialized polynomial vector size (bytes).  */
    uint16_t polyvec_compressed_bytes; /**< Compressed polynomial vector size (bytes).  */
    uint16_t poly_compressed_dv_bytes; /**< Compressed polynomial size for dv (bytes).  */
    uint16_t indcpa_pk_bytes;          /**< IND-CPA public key size (bytes).            */
    uint16_t indcpa_sk_bytes;          /**< IND-CPA secret key size (bytes).            */
    uint16_t indcpa_ct_bytes;          /**< IND-CPA ciphertext size (bytes).            */
    uint16_t pk_bytes;                 /**< ML-KEM public key size (bytes).             */
    uint16_t sk_bytes;                 /**< ML-KEM secret key size (bytes).             */
    uint16_t ct_bytes;                 /**< ML-KEM ciphertext size (bytes).             */
} MLKEM_param_info_t;

/**
 * @brief   Lookup table of ML-KEM parameter sets indexed by #MLKEM_param_t.
 */
extern const MLKEM_param_info_t MLKEM_PARAM[MLKEM_NUM_PARAM_SETS];

/**
 * @brief   Generates an ML-KEM key pair.
 *
 * Uses internal randomness to produce the key pair.
 *
 * @param[out] pk      Public key output buffer.
 * @param[in]  pk_len  Size of the public key buffer in bytes.
 * @param[out] sk      Secret key output buffer.
 * @param[in]  sk_len  Size of the secret key buffer in bytes.
 * @param[in]  param   ML-KEM parameter set selector.
 *
 * @return             CX_OK on success, error code otherwise.
 */
cx_err_t MLKEM_crypto_kem_keypair(uint8_t      *pk,
                                  size_t        pk_len,
                                  uint8_t      *sk,
                                  size_t        sk_len,
                                  MLKEM_param_t param);

/**
 * @brief   ML-KEM encapsulation.
 *
 * Produces a ciphertext and a shared secret from a public key using
 * internal randomness.
 *
 * @param[out] ct      Ciphertext output buffer.
 * @param[in]  ct_len  Size of the ciphertext buffer in bytes.
 * @param[out] ss      Shared secret output buffer.
 * @param[in]  ss_len  Size of the shared secret buffer in bytes.
 * @param[in]  pk      Public key.
 * @param[in]  pk_len  Size of the public key in bytes.
 * @param[in]  param   ML-KEM parameter set selector.
 *
 * @return             CX_OK on success, error code otherwise.
 */
cx_err_t MLKEM_crypto_kem_enc(uint8_t       *ct,
                              size_t         ct_len,
                              uint8_t       *ss,
                              size_t         ss_len,
                              const uint8_t *pk,
                              size_t         pk_len,
                              MLKEM_param_t  param);

/**
 * @brief   ML-KEM decapsulation.
 *
 * Recovers the shared secret from a ciphertext and a secret key.
 *
 * @param[out] ss      Shared secret output buffer.
 * @param[in]  ss_len  Size of the shared secret buffer in bytes.
 * @param[in]  ct      Ciphertext.
 * @param[in]  ct_len  Size of the ciphertext in bytes.
 * @param[in]  sk      Secret key.
 * @param[in]  sk_len  Size of the secret key in bytes.
 * @param[in]  param   ML-KEM parameter set selector.
 *
 * @return             CX_OK on success, error code otherwise.
 */
cx_err_t MLKEM_crypto_kem_dec(uint8_t       *ss,
                              size_t         ss_len,
                              const uint8_t *ct,
                              size_t         ct_len,
                              const uint8_t *sk,
                              size_t         sk_len,
                              MLKEM_param_t  param);

#endif  // MLKEM_H
