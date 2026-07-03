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
 * @file    lcx_mldsa.h
 * @brief   ML-DSA (Module-Lattice Digital Signature Algorithm) public API.
 *
 * Provides the public constants, parameter structures, and function prototypes
 * for ML-DSA-44, ML-DSA-65, and ML-DSA-87 as specified in FIPS 204.
 *
 * ML-DSA-87 support requires the HAVE_MLDSA_87 compilation flag.
 */

#ifndef MLDSA_H
#define MLDSA_H

#include <stdint.h>
#include <stddef.h>
#include "cx_errors.h"

/** @defgroup mldsa_common Common ML-DSA Parameters (FIPS 204)
 *  @{
 */

#define MLDSA_N         256U    /**< Number of coefficients per polynomial.           */
#define MLDSA_Q         8380417 /**< Modulus q.                                     */
#define MLDSA_D         13U     /**< Dropped bits from t.                             */
#define MLDSA_SEEDBYTES 32U     /**< Size of seeds (bytes).                           */
#define MLDSA_CRHBYTES  64U     /**< Size of CRH output (bytes).                     */
#define MLDSA_TRBYTES   64U     /**< Size of tr hash (bytes).                        */
#define MLDSA_RNDBYTES  32U     /**< Size of signing randomness (bytes).              */

#define MLDSA_POLYT1_PACKEDBYTES 320U /**< Packed polynomial t1 size (bytes).        */
#define MLDSA_POLYT0_PACKEDBYTES 416U /**< Packed polynomial t0 size (bytes).        */

/** @} */

/** @defgroup mldsa44 ML-DSA-44 Parameters (k=4, l=4, eta=2)
 *  @{
 */

#define MLDSA44_K      4U                   /**< Module dimension k.           */
#define MLDSA44_L      4U                   /**< Module dimension l.           */
#define MLDSA44_ETA    2U                   /**< Secret key range parameter.   */
#define MLDSA44_TAU    39U                  /**< Challenge weight.             */
#define MLDSA44_BETA   78U                  /**< tau * eta.                    */
#define MLDSA44_GAMMA1 (1 << 17)            /**< y coefficient range.    */
#define MLDSA44_GAMMA2 ((MLDSA_Q - 1) / 88) /**< Low-order rounding range. */
#define MLDSA44_OMEGA  80U                  /**< Max number of ones in hint.   */

#define MLDSA44_CTILDEBYTES         32U  /**< Challenge hash size (bytes).    */
#define MLDSA44_POLYETA_PACKEDBYTES 96U  /**< Packed eta polynomial (bytes).  */
#define MLDSA44_POLYZ_PACKEDBYTES   576U /**< Packed z polynomial (bytes).    */
#define MLDSA44_POLYW1_PACKEDBYTES  192U /**< Packed w1 polynomial (bytes).   */

#define MLDSA44_POLYVECH_PACKEDBYTES (MLDSA44_OMEGA + MLDSA44_K) /**< 84 bytes. */

#define MLDSA44_PUBLICKEYBYTES \
    (MLDSA_SEEDBYTES + MLDSA44_K * MLDSA_POLYT1_PACKEDBYTES) /**< 1312 bytes. */

#define MLDSA44_SECRETKEYBYTES                                                      \
    (2U * MLDSA_SEEDBYTES + MLDSA_TRBYTES + MLDSA44_L * MLDSA44_POLYETA_PACKEDBYTES \
     + MLDSA44_K * MLDSA44_POLYETA_PACKEDBYTES                                      \
     + MLDSA44_K * MLDSA_POLYT0_PACKEDBYTES) /**< 2560 bytes. */

#define MLDSA44_SIGBYTES                                         \
    (MLDSA44_CTILDEBYTES + MLDSA44_L * MLDSA44_POLYZ_PACKEDBYTES \
     + MLDSA44_POLYVECH_PACKEDBYTES) /**< 2420 bytes. */

/** @} */

/** @defgroup mldsa65 ML-DSA-65 Parameters (k=6, l=5, eta=4)
 *  @{
 */

#define MLDSA65_K      6U                   /**< Module dimension k.           */
#define MLDSA65_L      5U                   /**< Module dimension l.           */
#define MLDSA65_ETA    4U                   /**< Secret key range parameter.   */
#define MLDSA65_TAU    49U                  /**< Challenge weight.             */
#define MLDSA65_BETA   196U                 /**< tau * eta.                    */
#define MLDSA65_GAMMA1 (1 << 19)            /**< y coefficient range.    */
#define MLDSA65_GAMMA2 ((MLDSA_Q - 1) / 32) /**< Low-order rounding range. */
#define MLDSA65_OMEGA  55U                  /**< Max number of ones in hint.   */

#define MLDSA65_CTILDEBYTES         48U  /**< Challenge hash size (bytes).    */
#define MLDSA65_POLYETA_PACKEDBYTES 128U /**< Packed eta polynomial (bytes).  */
#define MLDSA65_POLYZ_PACKEDBYTES   640U /**< Packed z polynomial (bytes).    */
#define MLDSA65_POLYW1_PACKEDBYTES  128U /**< Packed w1 polynomial (bytes).   */

#define MLDSA65_POLYVECH_PACKEDBYTES (MLDSA65_OMEGA + MLDSA65_K) /**< 61 bytes. */

#define MLDSA65_PUBLICKEYBYTES \
    (MLDSA_SEEDBYTES + MLDSA65_K * MLDSA_POLYT1_PACKEDBYTES) /**< 1952 bytes. */

#define MLDSA65_SECRETKEYBYTES                                                      \
    (2U * MLDSA_SEEDBYTES + MLDSA_TRBYTES + MLDSA65_L * MLDSA65_POLYETA_PACKEDBYTES \
     + MLDSA65_K * MLDSA65_POLYETA_PACKEDBYTES                                      \
     + MLDSA65_K * MLDSA_POLYT0_PACKEDBYTES) /**< 4032 bytes. */

#define MLDSA65_SIGBYTES                                         \
    (MLDSA65_CTILDEBYTES + MLDSA65_L * MLDSA65_POLYZ_PACKEDBYTES \
     + MLDSA65_POLYVECH_PACKEDBYTES) /**< 3309 bytes. */

/** @} */

#ifdef HAVE_MLDSA_87
/** @defgroup mldsa87 ML-DSA-87 Parameters (k=8, l=7, eta=2)
 *  @{
 */

#define MLDSA87_K      8U                   /**< Module dimension k.           */
#define MLDSA87_L      7U                   /**< Module dimension l.           */
#define MLDSA87_ETA    2U                   /**< Secret key range parameter.   */
#define MLDSA87_TAU    60U                  /**< Challenge weight.             */
#define MLDSA87_BETA   120U                 /**< tau * eta.                    */
#define MLDSA87_GAMMA1 (1 << 19)            /**< y coefficient range.    */
#define MLDSA87_GAMMA2 ((MLDSA_Q - 1) / 32) /**< Low-order rounding range. */
#define MLDSA87_OMEGA  75U                  /**< Max number of ones in hint.   */

#define MLDSA87_CTILDEBYTES         64U  /**< Challenge hash size (bytes).    */
#define MLDSA87_POLYETA_PACKEDBYTES 96U  /**< Packed eta polynomial (bytes).  */
#define MLDSA87_POLYZ_PACKEDBYTES   640U /**< Packed z polynomial (bytes).    */
#define MLDSA87_POLYW1_PACKEDBYTES  128U /**< Packed w1 polynomial (bytes).   */

#define MLDSA87_POLYVECH_PACKEDBYTES (MLDSA87_OMEGA + MLDSA87_K) /**< 83 bytes. */

#define MLDSA87_PUBLICKEYBYTES \
    (MLDSA_SEEDBYTES + MLDSA87_K * MLDSA_POLYT1_PACKEDBYTES) /**< 2592 bytes. */

#define MLDSA87_SECRETKEYBYTES                                                      \
    (2U * MLDSA_SEEDBYTES + MLDSA_TRBYTES + MLDSA87_L * MLDSA87_POLYETA_PACKEDBYTES \
     + MLDSA87_K * MLDSA87_POLYETA_PACKEDBYTES                                      \
     + MLDSA87_K * MLDSA_POLYT0_PACKEDBYTES) /**< 4896 bytes. */

#define MLDSA87_SIGBYTES                                         \
    (MLDSA87_CTILDEBYTES + MLDSA87_L * MLDSA87_POLYZ_PACKEDBYTES \
     + MLDSA87_POLYVECH_PACKEDBYTES) /**< 4627 bytes. */

/** @} */
#endif /* HAVE_MLDSA_87 */

// Maximum dimensions across supported parameter sets.
#ifdef HAVE_MLDSA_87
#define MLDSA_MAX_K          MLDSA87_K /**< Maximum k.  */
#define MLDSA_MAX_L          MLDSA87_L /**< Maximum l.  */
#define MLDSA_NUM_PARAM_SETS 3U        /**< Number of supported parameter sets. */
#else
#define MLDSA_MAX_K          MLDSA65_K /**< Maximum k.  */
#define MLDSA_MAX_L          MLDSA65_L /**< Maximum l.  */
#define MLDSA_NUM_PARAM_SETS 2U        /**< Number of supported parameter sets. */
#endif

/**
 * @brief   ML-DSA parameter set selector.
 */
typedef enum MLDSA_param_e {
    MLDSA_44 = 0, /**< ML-DSA-44 (NIST security level 2).  */
    MLDSA_65,     /**< ML-DSA-65 (NIST security level 3).  */
#ifdef HAVE_MLDSA_87
    MLDSA_87, /**< ML-DSA-87 (NIST security level 5). */
#endif
} MLDSA_param_t;

/**
 * @brief   ML-DSA parameter set descriptor holding all derived sizes.
 */
typedef struct MLDSA_param_info_s {
    uint8_t  k;                     /**< Module dimension k.                        */
    uint8_t  l;                     /**< Module dimension l.                        */
    uint8_t  eta;                   /**< Secret key range parameter.                */
    uint8_t  tau;                   /**< Challenge weight.                          */
    uint16_t beta;                  /**< tau * eta.                                 */
    int32_t  gamma1;                /**< y coefficient range.                       */
    int32_t  gamma2;                /**< Low-order rounding range.                  */
    uint8_t  omega;                 /**< Max number of ones in hint.                */
    uint8_t  ctilde_bytes;          /**< Challenge hash size (bytes).               */
    uint16_t polyeta_packed_bytes;  /**< Packed eta polynomial size (bytes).        */
    uint16_t polyz_packed_bytes;    /**< Packed z polynomial size (bytes).          */
    uint16_t polyw1_packed_bytes;   /**< Packed w1 polynomial size (bytes).         */
    uint16_t polyvech_packed_bytes; /**< Packed hint vector size (bytes).           */
    uint16_t pk_bytes;              /**< Public key size (bytes).                   */
    uint16_t sk_bytes;              /**< Secret key size (bytes).                   */
    uint16_t sig_bytes;             /**< Signature size (bytes).                    */
} MLDSA_param_info_t;

/**
 * @brief   Lookup table of ML-DSA parameter sets indexed by #MLDSA_param_t.
 */
extern const MLDSA_param_info_t MLDSA_PARAM[MLDSA_NUM_PARAM_SETS];

/**
 * @brief   Generates an ML-DSA key pair.
 *
 * Uses internal randomness to produce the key pair.
 *
 * @param[out] pk      Public key output buffer.
 * @param[in]  pk_len  Size of the public key buffer in bytes.
 * @param[out] sk      Secret key output buffer.
 * @param[in]  sk_len  Size of the secret key buffer in bytes.
 * @param[in]  param   ML-DSA parameter set selector.
 *
 * @return             CX_OK on success, error code otherwise.
 */
cx_err_t MLDSA_keygen(uint8_t *pk, size_t pk_len, uint8_t *sk, size_t sk_len, MLDSA_param_t param);

/**
 * @brief   ML-DSA signature generation.
 *
 * @param[out] sig      Signature output buffer.
 * @param[in]  sig_len  Size of the signature buffer in bytes.
 * @param[out] sig_actual_len  Actual signature length written.
 * @param[in]  msg      Message to sign.
 * @param[in]  msg_len  Message length in bytes.
 * @param[in]  ctx      Context string (may be NULL if ctx_len == 0).
 * @param[in]  ctx_len  Context string length (must be <= 255).
 * @param[in]  sk       Secret key.
 * @param[in]  sk_len   Secret key length in bytes.
 * @param[in]  param    ML-DSA parameter set selector.
 *
 * @return              CX_OK on success, error code otherwise.
 */
cx_err_t MLDSA_sign(uint8_t       *sig,
                    size_t         sig_len,
                    size_t        *sig_actual_len,
                    const uint8_t *msg,
                    size_t         msg_len,
                    const uint8_t *ctx,
                    size_t         ctx_len,
                    const uint8_t *sk,
                    size_t         sk_len,
                    MLDSA_param_t  param);

/**
 * @brief   ML-DSA signature verification.
 *
 * @param[in]  sig      Signature to verify.
 * @param[in]  sig_len  Signature length in bytes.
 * @param[in]  msg      Message that was signed.
 * @param[in]  msg_len  Message length in bytes.
 * @param[in]  ctx      Context string (may be NULL if ctx_len == 0).
 * @param[in]  ctx_len  Context string length (must be <= 255).
 * @param[in]  pk       Public key.
 * @param[in]  pk_len   Public key length in bytes.
 * @param[in]  param    ML-DSA parameter set selector.
 *
 * @return              CX_OK if signature is valid, error code otherwise.
 */
cx_err_t MLDSA_verify(const uint8_t *sig,
                      size_t         sig_len,
                      const uint8_t *msg,
                      size_t         msg_len,
                      const uint8_t *ctx,
                      size_t         ctx_len,
                      const uint8_t *pk,
                      size_t         pk_len,
                      MLDSA_param_t  param);

/** @defgroup mldsa_prehash Pre-hash identifiers for HashML-DSA (FIPS 204, Section 5.4)
 *  @{
 */

/** Length of a pre-hash algorithm OID (DER-encoded, bytes). */
#define MLDSA_PREHASH_OID_LEN 11U

/**
 * @brief   Hash algorithm selector for HashML-DSA pre-hash signatures.
 */
typedef enum MLDSA_prehash_e {
    MLDSA_PREHASH_SHA256 = 0, /**< SHA-256   (32-byte output). */
    MLDSA_PREHASH_SHA512,     /**< SHA-512   (64-byte output). */
    MLDSA_PREHASH_SHA3_256,   /**< SHA3-256  (32-byte output). */
    MLDSA_PREHASH_SHA3_512,   /**< SHA3-512  (64-byte output). */
    MLDSA_PREHASH_SHAKE128,   /**< SHAKE128  (32-byte output). */
    MLDSA_PREHASH_SHAKE256,   /**< SHAKE256  (64-byte output). */
    MLDSA_NUM_PREHASH_ALGS    /**< Number of supported algorithms. */
} MLDSA_prehash_t;

/** @} */

/**
 * @brief   HashML-DSA pre-hash signature generation (FIPS 204, Algorithm 4).
 *
 * Signs a pre-hashed message using the HashML-DSA variant.  The caller
 * is responsible for hashing the message with the chosen algorithm before
 * calling this function.
 *
 * @param[out] sig             Signature output buffer.
 * @param[in]  sig_len         Size of the signature buffer in bytes.
 * @param[out] sig_actual_len  Actual signature length written.
 * @param[in]  ph              Pre-hashed message digest.
 * @param[in]  ph_len          Digest length (must match the hash algorithm).
 * @param[in]  ctx             Context string (may be NULL if ctx_len == 0).
 * @param[in]  ctx_len         Context string length (must be <= 255).
 * @param[in]  sk              Secret key.
 * @param[in]  sk_len          Secret key length in bytes.
 * @param[in]  prehash_alg     Pre-hash algorithm selector.
 * @param[in]  param           ML-DSA parameter set selector.
 *
 * @return                     CX_OK on success, error code otherwise.
 */
cx_err_t MLDSA_sign_prehash(uint8_t        *sig,
                            size_t          sig_len,
                            size_t         *sig_actual_len,
                            const uint8_t  *ph,
                            size_t          ph_len,
                            const uint8_t  *ctx,
                            size_t          ctx_len,
                            const uint8_t  *sk,
                            size_t          sk_len,
                            MLDSA_prehash_t prehash_alg,
                            MLDSA_param_t   param);

/**
 * @brief   HashML-DSA pre-hash signature verification (FIPS 204, Algorithm 5).
 *
 * Verifies a signature over a pre-hashed message using the HashML-DSA
 * variant.  The caller is responsible for hashing the message with the
 * chosen algorithm before calling this function.
 *
 * @param[in]  sig             Signature to verify.
 * @param[in]  sig_len         Signature length in bytes.
 * @param[in]  ph              Pre-hashed message digest.
 * @param[in]  ph_len          Digest length (must match the hash algorithm).
 * @param[in]  ctx             Context string (may be NULL if ctx_len == 0).
 * @param[in]  ctx_len         Context string length (must be <= 255).
 * @param[in]  pk              Public key.
 * @param[in]  pk_len          Public key length in bytes.
 * @param[in]  prehash_alg     Pre-hash algorithm selector.
 * @param[in]  param           ML-DSA parameter set selector.
 *
 * @return                     CX_OK if signature is valid, error code otherwise.
 */
cx_err_t MLDSA_verify_prehash(const uint8_t  *sig,
                              size_t          sig_len,
                              const uint8_t  *ph,
                              size_t          ph_len,
                              const uint8_t  *ctx,
                              size_t          ctx_len,
                              const uint8_t  *pk,
                              size_t          pk_len,
                              MLDSA_prehash_t prehash_alg,
                              MLDSA_param_t   param);

#endif /* MLDSA_H */
