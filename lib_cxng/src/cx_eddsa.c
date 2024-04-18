
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

#ifdef HAVE_ECC_TWISTED_EDWARDS

#include "cx_hash.h"
#include "cx_ecfp.h"
#include "cx_eddsa.h"
#include "cx_selftests.h"
#include "cx_utils.h"
#include "cx_ram.h"

#include <string.h>

#define EDDSA_ED448_DOMAIN_LEN (57)

static void cx_encode_int(uint8_t *v, int len)
{
    uint8_t t;
    int     i, j;
    i   = 0;
    j   = len - 1;
    len = len / 2;
    while (len--) {
        t    = v[i];
        v[i] = v[j];
        v[j] = t;
        i++;
        j--;
    }
}

#define cx_decode_int(v, l) cx_encode_int(v, l)

int cx_decode_coord(uint8_t *coord, int len)
{
    int sign;
    cx_encode_int(coord, len);  // little endian
    sign = coord[0] & 0x80;     // x_0
    coord[0] &= 0x7F;           // y-coordinate
    return sign;
}

void cx_encode_coord(uint8_t *coord, int len, int sign)
{
    coord[0] |= sign ? 0x80 : 0x00;
    cx_encode_int(coord, len);
}

/* ----------------------------------------------------------------------- */
/*                                                                         */
/* ----------------------------------------------------------------------- */
cx_err_t cx_eddsa_get_public_key_internal(const cx_ecfp_private_key_t *pv_key,
                                          cx_md_t                      hashID,
                                          cx_ecfp_public_key_t        *pu_key,
                                          uint8_t                     *a,
                                          size_t                       a_len,
                                          uint8_t                     *h,
                                          size_t                       h_len,
                                          uint8_t                     *scal /*tmp*/)
{
    size_t   size;
    cx_err_t error;

    CX_CHECK(cx_ecdomain_parameters_length(pv_key->curve, &size));

    if (!CX_CURVE_RANGE(pv_key->curve, TWISTED_EDWARDS)) {
        return CX_INVALID_PARAMETER;
    }
    if (!((pv_key->d_len == size) || (pv_key->d_len == (2 * size)))) {
        return CX_INVALID_PARAMETER;
    }
    if (!(((a == NULL) && (a_len == 0)) || ((a_len) && (a_len >= size)))) {
        return CX_INVALID_PARAMETER;
    }
    if (!(((h == NULL) && (h_len == 0)) || ((h_len) && (h_len >= size)))) {
        return CX_INVALID_PARAMETER;
    }

    switch (hashID) {
#if defined(HAVE_SHA512)
        case CX_SHA512:
#endif  // HAVE_SHA512

#if defined(HAVE_SHA3)
        case CX_SHAKE256:
        case CX_KECCAK:
        case CX_SHA3:
#endif  // HAVE_SHA3

#if defined(HAVE_BLAKE2)
        case CX_BLAKE2B:
            if (cx_hash_init_ex(&G_cx.hash_ctx, hashID, size * 2) != CX_OK) {
                return CX_INVALID_PARAMETER;
            }
            break;
#endif  // HAVE_BLAKE2

        default:
            return CX_INVALID_PARAMETER;
    }

    if (pv_key->d_len == size) {
        /* 1. Hash the 32/57-byte private key using SHA-512/shak256-114, storing the digest in
         * a 32/114 bytes large buffer, denoted h.  Only the lower [CME: first] 32/57 bytes are
         * used for generating the public key.
         */
        CX_CHECK(cx_hash_update(&G_cx.hash_ctx, pv_key->d, pv_key->d_len));
        CX_CHECK(cx_hash_final(&G_cx.hash_ctx, scal));
        cx_hash_destroy(&G_cx.hash_ctx);
        if (pv_key->curve == CX_CURVE_Ed25519) {
            /* 2. Prune the buffer: The lowest 3 bits of the first octet are
             * cleared, the highest bit of the last octet is cleared, and the
             * second highest bit of the last octet is set.
             */
            scal[0] &= 0xF8;
            scal[31] = (scal[31] & 0x7F) | 0x40;
        }
        else /* CX_CURVE_Ed448 */ {
            /* 2. Prune the buffer: The two least significant bits of the first
             * octet are cleared, all eight bits the last octet are cleared, and
             * the highest bit of the second to last octet is set.
             */
            scal[0] &= 0xFC;
            scal[56] = 0;
            scal[55] |= 0x80;
        }
    }
    else {
        memmove(scal, pv_key->d, pv_key->d_len);
    }

    /* 3. Interpret the buffer as the little-endian integer, forming a
     * secret scalar a.  Perform a fixed-base scalar multiplication
     * [a]B.
     */
    cx_decode_int(scal, size);
    if (a) {
        memmove(a, scal, size);
    }
    if (h) {
        memmove(h, scal + size, size);
    }
    if (pu_key) {
        cx_ecpoint_t W;
        CX_CHECK(cx_ecpoint_alloc(&W, pv_key->curve));
        CX_CHECK(cx_ecdomain_generator_bn(pv_key->curve, &W));
        CX_CHECK(cx_ecpoint_rnd_scalarmul(&W, scal, size));
        pu_key->curve = pv_key->curve;
        pu_key->W_len = 1 + 2 * size;
        pu_key->W[0]  = 0x04;
        CX_CHECK(cx_ecpoint_export(&W, pu_key->W + 1, size, pu_key->W + 1 + size, size));
        CX_CHECK(cx_ecpoint_destroy(&W));
    }

end:
    return error;
}

cx_err_t cx_eddsa_get_public_key_no_throw(const cx_ecfp_private_key_t *pv_key,
                                          cx_md_t                      hashID,
                                          cx_ecfp_public_key_t        *pu_key,
                                          uint8_t                     *a,
                                          size_t                       a_len,
                                          uint8_t                     *h,
                                          size_t                       h_len)
{
    uint8_t  scal[EDDSA_ED448_DOMAIN_LEN * 2];
    size_t   size;
    cx_err_t error;

    CX_CHECK(cx_ecdomain_parameters_length(pv_key->curve, &size));
    CX_CHECK(cx_bn_lock(size, 0));
    CX_CHECK(cx_eddsa_get_public_key_internal(pv_key, hashID, pu_key, a, a_len, h, h_len, scal));

end:
    cx_bn_unlock();
    return error;
}

#ifdef HAVE_EDDSA

static uint8_t const C_cx_siged448[] = {'S', 'i', 'g', 'E', 'd', '4', '4', '8'};

static cx_err_t cx_eddsa_check_params(cx_curve_t curve, size_t domain_len, cx_md_t hash_id)
{
    if (!CX_CURVE_RANGE(curve, TWISTED_EDWARDS)) {
        return CX_INVALID_PARAMETER;
    }

    // The hash digest must be 2 * domain_len bytes
    // Typically SHA512 for Ed25519 and SHAKE-256
    // with 114 bytes for Ed448
    switch (hash_id) {
#if (defined(HAVE_SHA512) || defined(HAVE_SHA3))
#if defined(HAVE_SHA512)
        case CX_SHA512:
#endif  // HAVE_SHA512

#if defined(HAVE_SHA3)
        case CX_KECCAK:
        case CX_SHA3:
#endif  // HAVE_SHA3
            if (domain_len * 2 != 512 / 8) {
                return INVALID_PARAMETER;
            }
            break;
#endif  // (defined(HAVE_SHA512) || defined(HAVE_SHA3))

#if (defined(HAVE_SHA3) || defined(HAVE_BLAKE2))
#if defined(HAVE_SHA3)
        case CX_SHAKE256:
#endif  // HAVE_SHA3

#if defined(HAVE_BLAKE2)
        case CX_BLAKE2B:
#endif  // HAVE_BLAKE2
            break;
#endif  // (defined(HAVE_SHA3) || defined(HAVE_BLAKE2))

        default:
            return INVALID_PARAMETER;
    }

    return CX_OK;
}

/* ----------------------------------------------------------------------- */
/*                                                                         */
/* ----------------------------------------------------------------------- */

cx_err_t cx_eddsa_sign_init_first_hash(cx_hash_t                   *hash_context,
                                       const cx_ecfp_private_key_t *private_key,
                                       cx_md_t                      hash_id)
{
    size_t   domain_len, hash_len;
    cx_err_t error;
    uint8_t  prefix[EDDSA_ED448_DOMAIN_LEN];
    uint8_t  private_hash[EDDSA_ED448_DOMAIN_LEN * 2];

    CX_CHECK(cx_ecdomain_parameters_length(private_key->curve, &domain_len));
    CX_CHECK(cx_eddsa_check_params(private_key->curve, domain_len, hash_id));

    if (!((private_key->d_len == domain_len) || (private_key->d_len == 2 * domain_len))) {
        return CX_INVALID_PARAMETER;
    }

    hash_len = 2 * domain_len;

    // retrieve the prefix from private key hash
    CX_CHECK(cx_eddsa_get_public_key_internal(
        private_key, hash_id, NULL, NULL, 0, prefix, sizeof(prefix), private_hash));

    CX_CHECK(cx_hash_init_ex(hash_context, hash_id, hash_len));

    // Hash dom4 for Ed448
    if (private_key->curve == CX_CURVE_Ed448) {
        CX_CHECK(cx_hash_update(hash_context, C_cx_siged448, sizeof(C_cx_siged448)));
        private_hash[0] = 0;
        private_hash[1] = 0;
        CX_CHECK(cx_hash_update(hash_context, private_hash, 2));
    }
    // Hash the prefix
    CX_CHECK(cx_hash_update(hash_context, prefix, domain_len));

end:
    return error;
}

cx_err_t cx_eddsa_sign_init_second_hash(cx_hash_t                   *hash_context,
                                        const cx_ecfp_private_key_t *private_key,
                                        cx_md_t                      hash_id,
                                        uint8_t                     *hash,
                                        size_t                       hash_len,
                                        uint8_t                     *sig,
                                        size_t                       sig_len)
{
    size_t       domain_len;
    cx_bn_t      bn_h, bn_r, bn_n;
    cx_ecpoint_t Q;
    cx_err_t     error;
    uint8_t      secret_scalar[EDDSA_ED448_DOMAIN_LEN];
    uint8_t      private_hash[EDDSA_ED448_DOMAIN_LEN * 2];
    uint32_t     sign;

    CX_CHECK(cx_ecdomain_parameters_length(private_key->curve, &domain_len));

    if (sig_len < 2 * domain_len) {
        return CX_INVALID_PARAMETER;
    }

    // Retrieve the secret scalar from the private key hash
    CX_CHECK(cx_eddsa_get_public_key_internal(
        private_key, hash_id, NULL, secret_scalar, sizeof(secret_scalar), NULL, 0, private_hash));
    // Encode the digest as little-endian
    cx_encode_int(hash, hash_len);

    CX_CHECK(cx_bn_lock(domain_len, 0));
    CX_CHECK(cx_bn_alloc_init(&bn_h, hash_len, hash, hash_len));
    CX_CHECK(cx_bn_alloc(&bn_r, domain_len));
    CX_CHECK(cx_bn_alloc(&bn_n, domain_len));
    CX_CHECK(cx_ecpoint_alloc(&Q, private_key->curve));
    CX_CHECK(cx_ecdomain_parameter_bn(private_key->curve, CX_CURVE_PARAM_Order, bn_n));
    CX_CHECK(cx_bn_reduce(bn_r, bn_h, bn_n));
    CX_CHECK(cx_bn_destroy(&bn_h));

    // Compute A = a.B (B group generator)
    CX_CHECK(cx_ecdomain_generator_bn(private_key->curve, &Q));
    CX_CHECK(cx_ecpoint_rnd_scalarmul(&Q, secret_scalar, domain_len));
    CX_CHECK(cx_ecpoint_compress(&Q, sig + domain_len, domain_len, &sign));
    // Temporary store compressed A into sig + domain_len
    cx_encode_coord(sig + domain_len, domain_len, sign);

    // Compute R = r.B
    // Use secret_scalar to temporary store r = hash mod n
    CX_CHECK(cx_bn_export(bn_r, secret_scalar, domain_len));
    CX_CHECK(cx_ecdomain_generator_bn(private_key->curve, &Q));
    CX_CHECK(cx_ecpoint_rnd_scalarmul(&Q, secret_scalar, domain_len));
    // Temporary store compressed R into sig
    CX_CHECK(cx_ecpoint_compress(&Q, sig, domain_len, &sign));
    cx_encode_coord(sig, domain_len, sign);

    // Compute H(dom || R || A) and update the message M to the hash context later
    CX_CHECK(cx_hash_init_ex(hash_context, hash_id, hash_len));
    // Ed448 dom4
    if (private_key->curve == CX_CURVE_Ed448) {
        CX_CHECK(cx_hash_update(hash_context, C_cx_siged448, sizeof(C_cx_siged448)));
        private_hash[0] = 0;
        private_hash[1] = 0;
        CX_CHECK(cx_hash_update(hash_context, private_hash, 2));
    }
    CX_CHECK(cx_hash_update(hash_context, sig, domain_len));
    CX_CHECK(cx_hash_update(hash_context, sig + domain_len, domain_len));

    // Temporary store r into sig + domain_len
    memcpy(sig + domain_len, secret_scalar, domain_len);

end:
    cx_bn_unlock();
    return error;
}

cx_err_t cx_eddsa_sign_hash(const cx_ecfp_private_key_t *pv_key,
                            cx_md_t                      hash_id,
                            const uint8_t               *hash,
                            size_t                       hash_len,
                            uint8_t                     *sig,
                            size_t                       sig_len)
{
    size_t   domain_len;
    uint8_t  secret_scalar[EDDSA_ED448_DOMAIN_LEN];
    uint8_t  hash_in[EDDSA_ED448_DOMAIN_LEN * 2];
    cx_bn_t  bn_h, bn_a, bn_r, bn_s, bn_n;
    cx_err_t error;

    CX_CHECK(cx_ecdomain_parameters_length(pv_key->curve, &domain_len));

    if (sig_len < 2 * domain_len) {
        return CX_INVALID_PARAMETER;
    }

    if (hash_len > EDDSA_ED448_DOMAIN_LEN * 2) {
        return CX_INVALID_PARAMETER;
    }
    memcpy(hash_in, hash, hash_len);
    // Encode the digest as little-endian
    cx_encode_int(hash_in, hash_len);

    CX_CHECK(cx_bn_lock(domain_len, 0));
    CX_CHECK(cx_bn_alloc(&bn_s, domain_len));
    CX_CHECK(cx_bn_alloc_init(&bn_h, hash_len, hash_in, hash_len));
    CX_CHECK(cx_bn_alloc(&bn_n, domain_len));
    CX_CHECK(cx_bn_alloc(&bn_r, domain_len));
    CX_CHECK(cx_ecdomain_parameter_bn(pv_key->curve, CX_CURVE_PARAM_Order, bn_n));
    CX_CHECK(cx_bn_reduce(bn_r, bn_h, bn_n));
    CX_CHECK(cx_eddsa_get_public_key_internal(
        pv_key, hash_id, NULL, secret_scalar, sizeof(secret_scalar), NULL, 0, hash_in));
    CX_CHECK(cx_bn_alloc_init(&bn_a, domain_len, secret_scalar, domain_len));
    // k * s mod n
    CX_CHECK(cx_bn_mod_mul(bn_s, bn_r, bn_a, bn_n));
    // Get previously computed r from sig buffer
    CX_CHECK(cx_bn_init(bn_r, sig + domain_len, domain_len));
    // S = r + k *s mod n
    CX_CHECK(cx_bn_mod_add(bn_s, bn_s, bn_r, bn_n));
    CX_CHECK(cx_bn_set_u32(bn_r, 0));
    CX_CHECK(cx_bn_mod_sub(bn_s, bn_s, bn_r, bn_n));
    CX_CHECK(cx_bn_export(bn_s, sig + domain_len, domain_len));
    cx_encode_int(sig + domain_len, domain_len);

end:
    cx_bn_unlock();
    return error;
}

cx_err_t cx_eddsa_update_hash(cx_hash_t *hash_context, const uint8_t *msg, size_t msg_len)
{
    return cx_hash_update(hash_context, msg, msg_len);
}

cx_err_t cx_eddsa_final_hash(cx_hash_t *hash_context, uint8_t *hash, size_t hash_len)
{
    if (hash_len != cx_hash_get_size(hash_context)) {
        return CX_INVALID_PARAMETER;
    }
    return cx_hash_final(hash_context, hash);
}

cx_err_t cx_eddsa_sign_no_throw(const cx_ecfp_private_key_t *pv_key,
                                cx_md_t                      hashID,
                                const uint8_t               *hash,
                                size_t                       hash_len,
                                uint8_t                     *sig,
                                size_t                       sig_len)
{
    uint8_t    out_hash[EDDSA_ED448_DOMAIN_LEN * 2] = {0};
    size_t     out_hash_len                         = 0;
    cx_err_t   error;
    cx_hash_t *hash_context = &G_cx.hash_ctx;

    CX_CHECK(cx_eddsa_sign_init_first_hash(hash_context, pv_key, hashID));
    CX_CHECK(cx_eddsa_update_hash(hash_context, hash, hash_len));
    out_hash_len = cx_hash_get_size(hash_context);
    CX_CHECK(cx_eddsa_final_hash(hash_context, out_hash, out_hash_len));
    explicit_bzero(hash_context, sizeof(cx_hash_t));
    CX_CHECK(cx_eddsa_sign_init_second_hash(
        hash_context, pv_key, hashID, out_hash, out_hash_len, sig, sig_len));
    CX_CHECK(cx_eddsa_update_hash(hash_context, hash, hash_len));
    CX_CHECK(cx_eddsa_final_hash(hash_context, out_hash, out_hash_len));
    CX_CHECK(cx_eddsa_sign_hash(pv_key, hashID, out_hash, out_hash_len, sig, sig_len));

end:
    explicit_bzero(hash_context, sizeof(cx_hash_t));
    return error;
}

/* ----------------------------------------------------------------------- */
/*                                                                         */
/* ----------------------------------------------------------------------- */

cx_err_t cx_eddsa_verify_init_hash(cx_hash_t                  *hash_context,
                                   const cx_ecfp_public_key_t *public_key,
                                   cx_md_t                     hash_id,
                                   const uint8_t              *sig_r,
                                   size_t                      sig_r_len)
{
    size_t       domain_len, hash_len;
    uint8_t      scal[EDDSA_ED448_DOMAIN_LEN] = {0};
    uint32_t     sign;
    cx_ecpoint_t P;
    cx_err_t     error;

    CX_CHECK(cx_ecdomain_parameters_length(public_key->curve, &domain_len));

    CX_CHECK(cx_eddsa_check_params(public_key->curve, domain_len, hash_id));

    if (!((public_key->W_len == 1 + domain_len) || (public_key->W_len == 1 + 2 * domain_len))) {
        return CX_INVALID_PARAMETER;
    }

    if (sig_r_len != domain_len) {
        return CX_INVALID_PARAMETER;
    }

    hash_len = 2 * domain_len;

    CX_CHECK(cx_bn_lock(domain_len, 0));
    CX_CHECK(cx_ecpoint_alloc(&P, public_key->curve));
    CX_CHECK(cx_hash_init_ex(hash_context, hash_id, hash_len));
    if (CX_CURVE_Ed448 == public_key->curve) {
        CX_CHECK(cx_hash_update(hash_context, C_cx_siged448, sizeof(C_cx_siged448)));
        scal[0] = 0;
        scal[1] = 0;
        CX_CHECK(cx_hash_update(hash_context, scal, 2));
    }
    CX_CHECK(cx_hash_update(hash_context, sig_r, sig_r_len));
    if (public_key->W[0] == 0x04) {
        CX_CHECK(cx_ecpoint_init(
            &P, &public_key->W[1], domain_len, &public_key->W[1 + domain_len], domain_len));
        CX_CHECK(cx_ecpoint_compress(&P, scal, domain_len, &sign));
        cx_encode_coord(scal, domain_len, sign);
    }
    else {
        memmove(scal, &public_key->W[1], domain_len);
    }
    CX_CHECK(cx_hash_update(hash_context, scal, domain_len));

end:
    cx_bn_unlock();
    return error;
}

bool cx_eddsa_verify_hash(const cx_ecfp_public_key_t *public_key,
                          uint8_t                    *hash,
                          size_t                      hash_len,
                          const uint8_t              *signature,
                          size_t                      signature_len)
{
    size_t       domain_len;
    uint8_t      scal[EDDSA_ED448_DOMAIN_LEN];
    uint8_t      scal_left[EDDSA_ED448_DOMAIN_LEN];
    int          diff;
    bool         are_equal = false;
    bool         verified  = false;
    uint32_t     sign;
    cx_bn_t      bn_h, bn_rs, bn_n;
    cx_bn_t      bn_p, bn_y;
    cx_ecpoint_t P, R, right_point, left_point;
    cx_err_t     error;

    CX_CHECK(cx_ecdomain_parameters_length(public_key->curve, &domain_len));

    if (signature_len != 2 * domain_len) {
        return false;
    }

    cx_encode_int(hash, hash_len);

    CX_CHECK(cx_bn_lock(domain_len, 0));
    CX_CHECK(cx_bn_alloc(&bn_n, domain_len));
    CX_CHECK(cx_ecdomain_parameter_bn(public_key->curve, CX_CURVE_PARAM_Order, bn_n));
    CX_CHECK(cx_bn_alloc(&bn_rs, domain_len));
    CX_CHECK(cx_bn_alloc_init(&bn_h, hash_len, hash, hash_len));
    CX_CHECK(cx_bn_reduce(bn_rs, bn_h, bn_n));
    CX_CHECK(cx_bn_export(bn_rs, scal, domain_len));
    CX_CHECK(cx_ecpoint_alloc(&P, public_key->curve));
    CX_CHECK(cx_ecpoint_init(
        &P, &public_key->W[1], domain_len, &public_key->W[1 + domain_len], domain_len));
    CX_CHECK(cx_ecpoint_neg(&P));

    memmove(scal_left, signature + domain_len, domain_len);
    cx_decode_int(scal_left, domain_len);

    // The second half of the signature s must be in range 0 <= s < L to prevent
    // signature malleability.
    CX_CHECK(cx_bn_alloc_init(&bn_rs, domain_len, scal_left, domain_len));
    CX_CHECK(cx_bn_cmp(bn_rs, bn_n, &diff));
    if (diff >= 0) {
        goto end;
    }

    CX_CHECK(cx_ecpoint_alloc(&left_point, public_key->curve));
    CX_CHECK(cx_ecdomain_generator_bn(public_key->curve, &left_point));
    CX_CHECK(cx_ecpoint_alloc(&right_point, public_key->curve));
    CX_CHECK(cx_ecpoint_cmp(&left_point, &P, &are_equal));

    if (are_equal) {
        CX_CHECK(cx_ecpoint_scalarmul(&left_point, scal_left, domain_len));
        CX_CHECK(cx_ecpoint_scalarmul(&P, scal, domain_len));
        CX_CHECK(cx_ecpoint_add(&right_point, &left_point, &P));
    }
    else {
        CX_CHECK(cx_ecpoint_double_scalarmul(
            &right_point, &left_point, &P, scal_left, domain_len, scal, domain_len));
    }

    CX_CHECK(cx_ecpoint_alloc(&R, public_key->curve));
    memmove(scal, signature, domain_len);
    sign = cx_decode_coord(scal, domain_len);

    CX_CHECK(cx_bn_alloc(&bn_p, domain_len));
    CX_CHECK(cx_ecdomain_parameter_bn(public_key->curve, CX_CURVE_PARAM_Field, bn_p));

    // If the y coordinate is >= p then the decoding fails
    CX_CHECK(cx_bn_alloc(&bn_y, domain_len));
    CX_CHECK(cx_bn_init(bn_y, scal, domain_len));

    CX_CHECK(cx_bn_cmp(bn_y, bn_p, &diff));
    if (diff >= 0) {
        goto end;
    }

    CX_CHECK(cx_ecpoint_decompress(&R, scal, domain_len, sign));
    CX_CHECK(cx_ecpoint_destroy(&left_point));
    CX_CHECK(cx_ecpoint_destroy(&P));

    // Check the signature
    CX_CHECK(cx_ecpoint_cmp(&R, &right_point, &verified));

end:
    cx_bn_unlock();
    return error == CX_OK && verified;
}

bool cx_eddsa_verify_no_throw(const cx_ecfp_public_key_t *pu_key,
                              cx_md_t                     hashID,
                              const uint8_t              *hash,
                              size_t                      hash_len,
                              const uint8_t              *sig,
                              size_t                      sig_len)
{
    uint8_t    out_hash[EDDSA_ED448_DOMAIN_LEN * 2] = {0};
    size_t     out_hash_len                         = 0;
    cx_hash_t *hash_context                         = &G_cx.hash_ctx;
    bool       verified                             = false;
    cx_err_t   error                                = CX_INTERNAL_ERROR;

    CX_CHECK(cx_eddsa_verify_init_hash(hash_context, pu_key, hashID, sig, sig_len / 2));
    out_hash_len = cx_hash_get_size(hash_context);
    CX_CHECK(cx_eddsa_update_hash(hash_context, hash, hash_len));
    CX_CHECK(cx_eddsa_final_hash(hash_context, out_hash, out_hash_len));
    verified = cx_eddsa_verify_hash(pu_key, out_hash, out_hash_len, sig, sig_len);

end:
    explicit_bzero(&G_cx.hash_ctx, sizeof(G_cx.hash_ctx));
    return verified;
}

#endif  // HAVE_EDDSA
#endif  // HAVE_ECC_TWISTED_EDWARDS
