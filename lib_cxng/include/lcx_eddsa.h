
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
 * @file    lcx_eddsa.h
 * @brief   EDDSA (Edwards Curve Digital Signature Algorithm)
 *
 * EDDSA is a digital signature scheme relying on Edwards curves, especially
 * Ed25519 and Ed448. Refer to <a href="https://tools.ietf.org/html/rfc8032"> RFC8032 </a>
 * for more details.
 */

#ifndef LCX_EDDSA_H
#define LCX_EDDSA_H

#include "lcx_ecfp.h"
#include "lcx_wrappers.h"

#ifdef HAVE_EDDSA

/**
 * @brief   Signs a message digest.
 *
 * @details The signature is done according to the EDDSA specification
 *          <a href="https://tools.ietf.org/html/rfc8032"> RFC8032 </a>.
 *
 * @param[in]  pvkey    Private key.
 *                      This shall be initialized with #cx_ecfp_init_private_key_no_throw.
 *
 * @param[in]  hashID   Message digest algorithm identifier.
 *                      Algorithms supported:
 *                        - SHA512
 *                        - SHA3
 *                        - Keccak
 *
 * @param[in]  hash     Pointer to the message digest.
 *
 * @param[in]  hash_len Length of the digest.
 *
 * @param[out] sig      Buffer where to store the signature.
 *
 * @param[in]  sig_len  Length of the signature.
 *
 * @return              Error code:
 *                      - CX_OK on success
 *                      - CX_EC_INVALID_CURVE
 *                      - CX_INVALID_PARAMETER
 *                      - INVALID_PARAMETER
 *                      - CX_NOT_UNLOCKED
 *                      - CX_INVALID_PARAMETER_SIZE
 *                      - CX_MEMORY_FULL
 *                      - CX_NOT_LOCKED
 *                      - CX_INVALID_PARAMETER_SIZE
 *                      - CX_EC_INVALID_POINT
 *                      - CX_EC_INFINITE_POINT
 *                      - CX_INTERNAL_ERROR
 *                      - CX_INVALID_PARAMETER_VALUE
 */
WARN_UNUSED_RESULT cx_err_t cx_eddsa_sign_no_throw(const cx_ecfp_private_key_t *pvkey,
                                                   cx_md_t                      hashID,
                                                   const uint8_t               *hash,
                                                   size_t                       hash_len,
                                                   uint8_t                     *sig,
                                                   size_t                       sig_len);

/**
 * @deprecated
 * See #cx_eddsa_sign_no_throw
 */
DEPRECATED static inline size_t cx_eddsa_sign(const cx_ecfp_private_key_t *pvkey,
                                              int                          mode,
                                              cx_md_t                      hashID,
                                              const unsigned char         *hash,
                                              unsigned int                 hash_len,
                                              const unsigned char         *ctx,
                                              unsigned int                 ctx_len,
                                              unsigned char               *sig,
                                              unsigned int                 sig_len,
                                              unsigned int                *info)
{
    UNUSED(ctx);
    UNUSED(ctx_len);
    UNUSED(mode);
    UNUSED(info);

    CX_THROW(cx_eddsa_sign_no_throw(pvkey, hashID, hash, hash_len, sig, sig_len));

    size_t size;
    CX_THROW(cx_ecdomain_parameters_length(pvkey->curve, &size));

    return 2 * size;
}

/**
 * @brief   Verifies a signature.
 *
 * @details The verification is done according to the specification
 *          <a href="https://tools.ietf.org/html/rfc8032"> RFC8032 </a>.
 *
 * @param[in]  pukey    Public key.
 *                      This shall be initialized with #cx_ecfp_init_public_key_no_throw.
 *
 * @param[in]  hashID   Message digest algorithm identifier.
 *                      Algorithms supported:
 *                        - SHA512
 *                        - SHA3
 *                        - Keccak
 *
 * @param[in]  hash     Pointer to the message digest.
 *
 * @param[in]  hash_len Length of the digest.
 *
 * @param[out] sig      Pointer to the signature.
 *
 * @param[in]  sig_len  Length of the signature.
 *
 * @return              1 if the signature is verified, otherwise 0.
 */
WARN_UNUSED_RESULT bool cx_eddsa_verify_no_throw(const cx_ecfp_public_key_t *pukey,
                                                 cx_md_t                     hashID,
                                                 const uint8_t              *hash,
                                                 size_t                      hash_len,
                                                 const uint8_t              *sig,
                                                 size_t                      sig_len);

/**
 * @brief   Verifies a signature.
 *
 * @details The verification is done according to the specification
 *          <a href="https://tools.ietf.org/html/rfc8032"> RFC8032 </a>.
 *          This function throws an exception if the computation doesn't
 *          succeed.
 *
 * @param[in]  pukey    Public key.
 *                      THis shall be initialized with #cx_ecfp_init_public_key_no_throw.
 *
 * @param[in]  mode     Mode. This parameter is not used.
 *
 * @param[in]  hashID   Message digest algorithm identifier.
 *                      Algorithms supported:
 *                        - SHA512
 *                        - SHA3
 *                        - Keccak
 *
 * @param[in]  hash     Pointer to the message digest.
 *
 * @param[in]  hash_len Length of the digest.
 *
 * @param[in]  ctx      Pointer to the context. This parameter is not used.
 *
 * @param[in]  ctx_len  Length of the context. This parameter is not used.
 *
 * @param[out] sig      Pointer to the signature.
 *
 * @param[in]  sig_len  Length of the signature.
 *
 * @return              1 if the signature is verified, otherwise 0.
 */
static inline int cx_eddsa_verify(const cx_ecfp_public_key_t *pukey,
                                  int                         mode,
                                  cx_md_t                     hashID,
                                  const unsigned char        *hash,
                                  unsigned int                hash_len,
                                  const unsigned char        *ctx,
                                  unsigned int                ctx_len,
                                  const unsigned char        *sig,
                                  unsigned int                sig_len)
{
    UNUSED(mode);
    UNUSED(ctx);
    UNUSED(ctx_len);

    return cx_eddsa_verify_no_throw(pukey, hashID, hash, hash_len, sig, sig_len);
}

/**
 * @brief   Encodes the curve point coordinates.
 *
 * @param[in, out] coord A pointer to the point coordinates in the form x|y.
 *
 * @param[in]      len   Length of the coordinates.
 *
 * @param[in]      sign  Sign of the x-coordinate.
 *
 */
void cx_encode_coord(uint8_t *coord, int len, int sign);

/**
 * @brief   Decodes the curve point coordinates.
 *
 * @param[in, out] coord A pointer to the point encoded coordinates.
 *
 * @param[in]      len   Length of the encoded coordinates.
 *
 * @return               Sign of the x-coordinate.
 */
int cx_decode_coord(uint8_t *coord, int len);

/**
 * @brief Computes the first hash for edDSA signature: H(dom || prefix).
 *
 * @details This function must be followed by one or several calls to #cx_eddsa_update_hash
 *          to hash the message and by a call to #cx_eddsa_final_hash to get the digest
 *          H(dom || prefix || PH(M)).
 *
 * @param[in] hash_context Pointer to the hash context.
 * @param[in] private_key  Pointer to the private key structure.
 *                         The private key is used to compute the prefix.
 * @param[in] hash_id      Hash identifier.
 * @return Error code
 */
cx_err_t cx_eddsa_sign_init_first_hash(cx_hash_t                   *hash_context,
                                       const cx_ecfp_private_key_t *private_key,
                                       cx_md_t                      hash_id);

/**
 * @brief Computes the second hash for edDSA signature: H(dom || R || A).
 *
 * @details This function must be followed by one or several calls to #cx_eddsa_update_hash
 *          to hash the message and by a call to #cx_eddsa_final_hash to get the digest
 *          H(dom || R || A || PH(M)).
 *
 * @param[in]  hash_context Pointer to the hash context.
 * @param[in]  private_key  Pointer to the private key structure.
 *                          The private key is used to compute the secret scalar
 *                          and the prefix.
 * @param[in]  hash_id      Hash identifier.
 * @param[in]  hash         Pointer to hash returned by #cx_eddsa_final_hash.
 * @param[in]  hash_len     Digest length.
 * @param[out] sig          Pointer to the signature buffer.
 * @param[in]  sig_len      Signature length.
 * @return Error code
 */
cx_err_t cx_eddsa_sign_init_second_hash(cx_hash_t                   *hash_context,
                                        const cx_ecfp_private_key_t *private_key,
                                        cx_md_t                      hash_id,
                                        uint8_t                     *hash,
                                        size_t                       hash_len,
                                        uint8_t                     *sig,
                                        size_t                       sig_len);

/**
 * @brief Computes the edDSA signature given the previously computed hash.
 *
 * @param[in]  pv_key   Pointer to the private key structure.
 * @param[in]  hash_id  Hash identifier.
 * @param[in]  hash     Pointer to hash returned by #cx_eddsa_final_hash.
 * @param[in]  hash_len Hash length.
 * @param[out] sig      Pointer to the signature buffer.
 * @param[in]  sig_len  Signature length.
 * @return Error code
 */
cx_err_t cx_eddsa_sign_hash(const cx_ecfp_private_key_t *pv_key,
                            cx_md_t                      hash_id,
                            const uint8_t               *hash,
                            size_t                       hash_len,
                            uint8_t                     *sig,
                            size_t                       sig_len);

/**
 * @brief Computes the hash for edDSA verification: H(dom || R || A)
 *
 * @details This function must be followed by one or several calls to #cx_eddsa_update_hash
 *          to hash the message and by a call to #cx_eddsa_final_hash to get the digest
 *          H(dom || R || A || PH(M)).
 *
 * @param[in] hash_context Pointer to the hash context.
 * @param[in] public_key   Pointer to the public key structure.
 * @param[in] hash_id      Hash identifier.
 * @param[in] sig_r        Pointer to the first half (r value) of the signature
 * @param[in] sig_r_len    Length of sig_r (should be half of the signature length)
 * @return Error code
 */
cx_err_t cx_eddsa_verify_init_hash(cx_hash_t                  *hash_context,
                                   const cx_ecfp_public_key_t *public_key,
                                   cx_md_t                     hash_id,
                                   const uint8_t              *sig_r,
                                   size_t                      sig_r_len);

/**
 * @brief Updates the hash context with given message.
 *
 * @details This function can be called several times to hash more message. It must
 *          be followed by #cx_eddsa_final_hash to get the digest. #cx_eddsa_sign_init_first_hash,
 *          #cx_eddsa_sign_init_second_hash or #cx_eddsa_verify_init_hash must be called
 *          before this function.
 *          The input of this function can be the result of the pre-hash function over the message
 *          for HashEdDSA variants.
 *
 * @param[in] hash_context Pointer to the hash context.
 * @param[in] msg          Pointer to the message to hash.
 * @param[in] msg_len      Message length.
 * @return Error code
 */
cx_err_t cx_eddsa_update_hash(cx_hash_t *hash_context, const uint8_t *msg, size_t msg_len);

/**
 * @brief Returns the result hash for edDSA signature or verification.
 *
 * @details This function must be called after #cx_eddsa_update_hash when no more
 *          messages need to be hashed.
 *
 * @param[in]  hash_context Pointer to the hash context.
 * @param[out] hash         Pointer to the digest.
 * @param[in]  hash_len     Digest length
 * @return Error code
 */
cx_err_t cx_eddsa_final_hash(cx_hash_t *hash_context, uint8_t *hash, size_t hash_len);

/**
 * @brief Verifies an edDSA signature given the previously computed hash.
 *
 * @param[in] public_key    Pointer to the public key structure.
 * @param[in] hash          Pointer to the hash returned by #cx_eddsa_final_hash.
 * @param[in] hash_len      Digest length
 * @param[in] signature     Pointer to the signature
 * @param[in] signature_len Signature length
 * @return bool
 * @retval true  Signature verification succeeds
 * @retval false Signature verification fails
 */
bool cx_eddsa_verify_hash(const cx_ecfp_public_key_t *public_key,
                          uint8_t                    *hash,
                          size_t                      hash_len,
                          const uint8_t              *signature,
                          size_t                      signature_len);

#endif  // HAVE_EDDSA

#endif  // LCX_EDDSA_H
