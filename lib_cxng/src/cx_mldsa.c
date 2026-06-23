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
 * @file    cx_mldsa.c
 * @brief   ML-DSA digital signature algorithm (FIPS 204).
 *
 * Implements the top-level ML-DSA key generation, signing, and verification
 * routines as specified in FIPS 204 Algorithms 1, 2, 3, 4, 5, 6, 7, 8.
 */

/*********************
 *      INCLUDES
 *********************/

#include <string.h>
#include "lcx_mldsa.h"
#include "cx_mldsa_poly.h"
#include "cx_mldsa_polyvec.h"
#include "cx_mldsa_polymat.h"
#include "cx_mldsa_sample.h"
#include "cx_mldsa_packing.h"
#include "cx_mldsa_rounding.h"
#include "cx_mldsa_util.h"
#include "cx_mldsa_internal.h"
#include "lcx_sha3.h"
#include "lcx_hash.h"
#include "lcx_rng.h"
#ifdef HAVE_MLDSA_OPTIMIZATION
#include "cx_mldsa_smallpoly.h"
#include "cx_mldsa_lowram.h"
#endif

/*********************
 *      DEFINES
 *********************/

/**
 * Maximum number of signing attempts before failing.
 * With the FIPS 204 Appendix C bound (>= 814), probability < 2^{-256}.
 */
#define MLDSA_MAX_SIGN_ATTEMPTS 814U

/**********************
 *      TYPEDEFS
 **********************/

/**
 * @brief DER-encoded OID and expected output length for each pre-hash algorithm.
 * (FIPS 204, Section 5.4)
 *
 * Indexed by #MLDSA_prehash_t.  All OIDs share the NIST algorithm arc
 * 06 09 60 86 48 01 65 03 04 02 xx.
 */
typedef struct MLDSA_prehash_info_s {
    uint8_t oid[MLDSA_PREHASH_OID_LEN]; /**< DER-encoded OID.       */
    uint8_t hash_len;                   /**< Expected digest length. */
} MLDSA_prehash_info_t;

/**
 * Format structure of M' = 0x01 (1b)|| context_len (1b) || context (225b) || OID || PH_M.
 * (FIPS 204, Section 5.4)
 */
typedef struct MLDSA_formatted_message_s {
    uint8_t prefix[2U + 255U + MLDSA_PREHASH_OID_LEN]; /**< Prefix = 0x01 (1b)|| context_len (1b) ||
                                                          context (225b) || OID */
    size_t         prefix_len;                         /**< Prefix length */
    const uint8_t *payload;                            /**< Pre-hash PH_M */
    size_t         payload_len /**< Pre-hash length */;
} MLDSA_formatted_message_t;

/**
 * @brief Stack-allocated workspace for #MLDSA_internal_sign_core.
 *
 * The signer streams the secret key, the masking vector @c y and the matrix
 * @c A one polynomial at a time, so the only full vector kept across a
 * rejection-loop iteration is @c w.
 */
typedef struct MLDSA_sign_stack_workspace_s {
    uint8_t        rho[MLDSA_SEEDBYTES];          /**< Public seed rho.                         */
    uint8_t        K[MLDSA_SEEDBYTES];            /**< Secret seed K from the secret key.       */
    uint8_t        tr[MLDSA_TRBYTES];             /**< Hash of the public key (tr = H(pk)).     */
    uint8_t        mu[MLDSA_CRHBYTES];            /**< Message representative mu = H(tr || M'). */
    uint8_t        rhoprime[MLDSA_CRHBYTES];      /**< Derived nonce seed rhoprime.             */
    uint8_t        ctilde[64U];                   /**< Challenge hash c_tilde.                  */
    uint8_t        w1_packed[MLDSA_MAX_K * 192U]; /**< Byte-packed w1 vector for hashing.       */
    mldsa_polyveck w;                             /**< Product A*NTT(y), kept across iteration. */
    mldsa_poly     cp;                            /**< Challenge polynomial c (NTT domain).     */
    mldsa_poly     t0;                            /**< General-purpose scratch polynomial.      */
    mldsa_poly     t1;                            /**< General-purpose scratch polynomial.      */
    mldsa_poly     t2;                            /**< General-purpose scratch polynomial.      */
} MLDSA_sign_stack_workspace_t;

/**
 * @brief Phase-overlaid scratch union for #MLDSA_internal_verify_core.
 *
 * The two phases of the verification algorithm never execute concurrently:
 * - @c setup_phase: @c tr is only needed early to compute @c mu when no
 *   pre-computed value is supplied.
 * - @c az_phase: @c aij and @c dot are only used inside the Az product loop.
 *
 * Overlaying them in a union reduces peak stack consumption.
 */
typedef union {
    /** @brief Early-phase temporary used to derive mu. */
    struct {
        uint8_t tr[MLDSA_TRBYTES]; /**< Hash of the public key (tr = H(pk)). */
    } setup_phase;
    /** @brief Az product loop temporaries. */
    struct {
        mldsa_poly aij; /**< Expanded matrix element A[i][j].                  */
        mldsa_poly dot; /**< Running dot-product accumulator for row i of A*z. */
    } az_phase;
} MLDSA_verify_phase_overlay_t;

/**
 * @brief Stack-allocated workspace for #MLDSA_internal_verify_core.
 *
 * Groups all large intermediate buffers needed by the verification algorithm
 * so that a single local variable covers the entire workspace.
 */
typedef struct MLDSA_verify_stack_workspace_s {
    uint8_t    rho[MLDSA_SEEDBYTES];       /**< Public seed rho extracted from the public key.   */
    uint8_t    ctilde[64U];                /**< Challenge hash c_tilde from the signature.       */
    uint8_t    mu[MLDSA_CRHBYTES];         /**< Message representative mu = H(tr || M').         */
    mldsa_poly cp;                         /**< Challenge polynomial c (NTT domain).             */
    mldsa_poly ztmp;                       /**< Temporary for streamed z polynomial unpacking.    */
    mldsa_poly t1tmp;                      /**< Temporary for streamed t1 polynomial unpacking.   */
    mldsa_poly htmp;                       /**< Temporary for reconstructed hint polynomial.      */
    uint8_t w1_packed[MLDSA_MAX_K * 192U]; /**< Byte-packed reconstructed w1' vector.             */
    uint8_t ctilde2[64U];                  /**< Recomputed challenge hash for comparison.         */
    MLDSA_verify_phase_overlay_t overlay;  /**< Phase-overlaid scratch space.                     */
} MLDSA_verify_stack_workspace_t;

/*********************
 *  GLOBAL VARIABLES
 *********************/

/*********************
 *  STATIC VARIABLES
 *********************/

static const MLDSA_prehash_info_t MLDSA_PREHASH_INFO[MLDSA_NUM_PREHASH_ALGS] = {
  // MLDSA_PREHASH_SHA256
    {{0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x01}, 32U},
 // MLDSA_PREHASH_SHA512
    {{0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x03}, 64U},
 // MLDSA_PREHASH_SHA3_256
    {{0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x08}, 32U},
 // MLDSA_PREHASH_SHA3_512
    {{0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x0A}, 64U},
 // MLDSA_PREHASH_SHAKE128
    {{0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x0B}, 32U},
 // MLDSA_PREHASH_SHAKE256
    {{0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x0C}, 64U},
};

/*********************
 *  STATIC FUNCTIONS
 *********************/

/**
 * @brief Formats a pure (non-pre-hashed) message into M' (FIPS 204, Section 5.2).
 *
 * Builds M' = 0x00 || len(ctx) || ctx || msg.
 *
 * @param[out] mprime   Formatted message structure to populate.
 * @param[in]  ctx      Context string (may be NULL if @p ctx_len is 0).
 * @param[in]  ctx_len  Context string length in bytes (must be <= 255).
 * @param[in]  msg      Message to sign or verify (may be NULL if @p msg_len is 0).
 * @param[in]  msg_len  Message length in bytes.
 *
 * @return              CX_OK on success, error code otherwise.
 */
static cx_err_t mldsa_format_message_pure(MLDSA_formatted_message_t *mprime,
                                          const uint8_t             *ctx,
                                          size_t                     ctx_len,
                                          const uint8_t             *msg,
                                          size_t                     msg_len)
{
    if (mprime == NULL) {
        return CX_INVALID_PARAMETER;
    }
    if ((msg == NULL) && (msg_len > 0U)) {
        return CX_INVALID_PARAMETER;
    }
    if ((ctx == NULL) && (ctx_len > 0U)) {
        return CX_INVALID_PARAMETER;
    }
    if (ctx_len > 255U) {
        return CX_INVALID_PARAMETER_SIZE;
    }

    mprime->prefix[0] = 0x00U;
    mprime->prefix[1] = (uint8_t) ctx_len;
    if (ctx_len > 0U) {
        memcpy(&mprime->prefix[2], ctx, ctx_len);
    }
    mprime->prefix_len  = 2U + ctx_len;
    mprime->payload     = msg;
    mprime->payload_len = msg_len;

    return CX_OK;
}

/**
 * @brief Formats a pre-hashed message into M' for HashML-DSA (FIPS 204, Section 5.4).
 *
 * Builds M' = 0x01 || len(ctx) || ctx || OID(prehash_alg) || PH_M.
 *
 * @param[out] mprime       Formatted message structure to populate.
 * @param[in]  ctx          Context string (may be NULL if @p ctx_len is 0).
 * @param[in]  ctx_len      Context string length in bytes (must be <= 255).
 * @param[in]  prehash_alg  Pre-hash algorithm selector.
 * @param[in]  ph           Pre-hashed message digest.
 * @param[in]  ph_len       Digest length (must match the algorithm's output size).
 *
 * @return                  CX_OK on success, error code otherwise.
 */
static cx_err_t mldsa_format_message_prehash(MLDSA_formatted_message_t *mprime,
                                             const uint8_t             *ctx,
                                             size_t                     ctx_len,
                                             MLDSA_prehash_t            prehash_alg,
                                             const uint8_t             *ph,
                                             size_t                     ph_len)
{
    if (mprime == NULL) {
        return CX_INVALID_PARAMETER;
    }
    if ((ph == NULL) && (ph_len > 0U)) {
        return CX_INVALID_PARAMETER;
    }
    if ((ctx == NULL) && (ctx_len > 0U)) {
        return CX_INVALID_PARAMETER;
    }
    if (ctx_len > 255U) {
        return CX_INVALID_PARAMETER_SIZE;
    }

    if ((unsigned) prehash_alg >= MLDSA_NUM_PREHASH_ALGS) {
        return CX_INVALID_PARAMETER_VALUE;
    }
    if (ph_len != MLDSA_PREHASH_INFO[prehash_alg].hash_len) {
        return CX_INVALID_PARAMETER_SIZE;
    }

    mprime->prefix[0] = 0x01U;
    mprime->prefix[1] = (uint8_t) ctx_len;
    if (ctx_len > 0U) {
        memcpy(&mprime->prefix[2], ctx, ctx_len);
    }
    memcpy(
        &mprime->prefix[2U + ctx_len], MLDSA_PREHASH_INFO[prehash_alg].oid, MLDSA_PREHASH_OID_LEN);
    mprime->prefix_len  = 2U + ctx_len + MLDSA_PREHASH_OID_LEN;
    mprime->payload     = ph;
    mprime->payload_len = ph_len;

    return CX_OK;
}

/**
 * @brief   Computes mu = SHAKE256(tr || M', 64) for a formatted message M'.
 *
 * @param[out] mu         Output buffer (MLDSA_CRHBYTES bytes).
 * @param[in]  tr         tr value from secret / public key (MLDSA_TRBYTES bytes).
 * @param[in]  mprime     Formatted message M'.
 */
static void mldsa_compute_mu(uint8_t                          mu[MLDSA_CRHBYTES],
                             const uint8_t                    tr[MLDSA_TRBYTES],
                             const MLDSA_formatted_message_t *mprime)
{
    cx_sha3_t sha3_ctx = {0};

    memset(&sha3_ctx, 0, sizeof(sha3_ctx));
    cx_shake256_init_no_throw(&sha3_ctx, MLDSA_CRHBYTES * 8U);
    cx_hash_no_throw((cx_hash_t *) &sha3_ctx, 0, tr, MLDSA_TRBYTES, NULL, 0);
    cx_hash_no_throw((cx_hash_t *) &sha3_ctx, 0, mprime->prefix, mprime->prefix_len, NULL, 0);
    cx_hash_no_throw(
        (cx_hash_t *) &sha3_ctx, CX_LAST, mprime->payload, mprime->payload_len, mu, MLDSA_CRHBYTES);
}

#ifndef HAVE_MLDSA_OPTIMIZATION

/**
 * @brief Core ML-DSA signing routine (FIPS 204, Algorithms 2 & 7).
 *
 * Performs the rejection-sampling signing loop.  Accepts either a formatted
 * message @p formatted_mprime (from which mu is derived) or a pre-computed
 * @p precomputed_mu; exactly one must be non-NULL.
 *
 * @param[out] sig               Signature output buffer.
 * @param[in]  sig_len           Size of the signature buffer in bytes.
 * @param[out] sig_actual_len    Actual signature length written on success.
 * @param[in]  formatted_mprime  Formatted message M' (NULL when using @p precomputed_mu).
 * @param[in]  precomputed_mu    Pre-computed mu (NULL when using @p formatted_mprime).
 * @param[in]  rnd               Signing randomness (MLDSA_RNDBYTES), or NULL for deterministic
 * signing.
 * @param[in]  rnd_len           Length of @p rnd (must be MLDSA_RNDBYTES or 0 if NULL).
 * @param[in]  sk                Secret key.
 * @param[in]  sk_len            Secret key length in bytes.
 * @param[in]  param             ML-DSA parameter set selector.
 *
 * @return                       CX_OK on success, error code otherwise.
 */
cx_err_t MLDSA_internal_sign_core(uint8_t                         *sig,
                                  size_t                           sig_len,
                                  size_t                          *sig_actual_len,
                                  const MLDSA_formatted_message_t *formatted_mprime,
                                  const uint8_t                   *precomputed_mu,
                                  uint8_t                         *rnd,
                                  size_t                           rnd_len,
                                  const uint8_t                   *sk,
                                  size_t                           sk_len,
                                  MLDSA_param_t                    param)
{
    MLDSA_sign_stack_workspace_t  ws_local                 = {0};
    MLDSA_sign_stack_workspace_t *ws                       = &ws_local;
    const MLDSA_param_info_t     *p                        = NULL;
    const uint8_t                *rnd_input                = rnd;
    uint8_t                       zero_rnd[MLDSA_RNDBYTES] = {0};
    cx_err_t                      error                    = CX_INTERNAL_ERROR;
    uint16_t                      kappa                    = 0U;
    uint32_t                      attempts                 = 0U;

    if ((sig == NULL) || (sk == NULL) || (sig_actual_len == NULL)) {
        error = CX_INVALID_PARAMETER;
        goto cleanup;
    }
    if ((formatted_mprime == NULL) && (precomputed_mu == NULL)) {
        error = CX_INVALID_PARAMETER;
        goto cleanup;
    }

    if ((rnd != NULL) && (rnd_len != MLDSA_RNDBYTES)) {
        error = CX_INVALID_PARAMETER;
        goto cleanup;
    }
    if ((rnd == NULL) && (rnd_len != 0U)) {
        error = CX_INVALID_PARAMETER;
        goto cleanup;
    }

    if (param >= MLDSA_NUM_PARAM_SETS) {
        error = CX_INVALID_PARAMETER_VALUE;
        goto cleanup;
    }

    p = &MLDSA_PARAM[param];

    if (sk_len < p->sk_bytes) {
        error = CX_INVALID_PARAMETER_SIZE;
        goto cleanup;
    }
    if (sig_len < p->sig_bytes) {
        error = CX_INVALID_PARAMETER_SIZE;
        goto cleanup;
    }

    if (rnd_input == NULL) {
        rnd_input = zero_rnd;
    }

    explicit_bzero(ws, sizeof(*ws));

    // Unpack secret-key header only; large vectors are decoded on the fly.
    memcpy(ws->rho, sk, MLDSA_SEEDBYTES);
    memcpy(ws->K, &sk[MLDSA_SEEDBYTES], MLDSA_SEEDBYTES);
    memcpy(ws->tr, &sk[2U * MLDSA_SEEDBYTES], MLDSA_TRBYTES);

    const uint8_t *sk_s1 = &sk[2U * MLDSA_SEEDBYTES + MLDSA_TRBYTES];
    const uint8_t *sk_s2 = &sk_s1[(size_t) p->l * p->polyeta_packed_bytes];
    const uint8_t *sk_t0 = &sk_s2[(size_t) p->k * p->polyeta_packed_bytes];

    // Compute or import mu
    if (precomputed_mu != NULL) {
        memcpy(ws->mu, precomputed_mu, MLDSA_CRHBYTES);
    }
    else {
        mldsa_compute_mu(ws->mu, ws->tr, formatted_mprime);
    }

    // Compute rhoprime = H(K || rnd || mu, 64)
    {
        cx_sha3_t sha3_ctx = {0};
        cx_shake256_init_no_throw(&sha3_ctx, MLDSA_CRHBYTES * 8U);
        cx_hash_no_throw((cx_hash_t *) &sha3_ctx, 0, ws->K, MLDSA_SEEDBYTES, NULL, 0);
        cx_hash_no_throw((cx_hash_t *) &sha3_ctx, 0, rnd_input, MLDSA_RNDBYTES, NULL, 0);
        cx_hash_no_throw(
            (cx_hash_t *) &sha3_ctx, CX_LAST, ws->mu, MLDSA_CRHBYTES, ws->rhoprime, MLDSA_CRHBYTES);
    }

    // Rejection sampling loop
    while (attempts < MLDSA_MAX_SIGN_ATTEMPTS) {
        uint16_t kappa_base = kappa;
        attempts++;

        // w = A * NTT(y) streamed one column at a time.
        // For each l: sample y[l], NTT it (t0 = yhat[l]), then accumulate
        // A[k][l] * yhat[l] into every row w[k]. Only one yhat poly is alive.
        for (uint8_t l = 0U; l < p->l; l++) {
            MLDSA_SAMPLE_gamma1(&ws->t0, ws->rhoprime, (uint16_t) (kappa_base + l), p->gamma1);
            MLDSA_POLY_ntt(&ws->t0);
            for (uint8_t k = 0U; k < p->k; k++) {
                uint16_t nonce = ((uint16_t) k << 8U) | (uint16_t) l;
                MLDSA_SAMPLE_uniform(&ws->t1, ws->rho, nonce);
                MLDSA_POLY_pointwise_montgomery(&ws->w.vec[k], &ws->t1, &ws->t0, (l == 0U) ? 1 : 0);
            }
        }
        kappa = (uint16_t) (kappa_base + p->l);

        MLDSA_POLYVEC_reduce_k(&ws->w, p->k);
        MLDSA_POLYVEC_invntt_tomont_k(&ws->w, p->k);
        MLDSA_POLYVEC_caddq_k(&ws->w, p->k);

        // Pack w1 = HighBits(w) one row at a time.
        for (uint8_t k = 0U; k < p->k; k++) {
            MLDSA_ROUNDING_poly_decompose(&ws->t0, &ws->t1, &ws->w.vec[k], p->gamma2);
            MLDSA_PACK_polyw1(
                &ws->w1_packed[(size_t) k * p->polyw1_packed_bytes], &ws->t0, p->gamma2);
        }

        // Challenge hash ctilde = H(mu || w1_packed)
        MLDSA_UTIL_shake256_two(ws->ctilde,
                                p->ctilde_bytes,
                                ws->mu,
                                MLDSA_CRHBYTES,
                                ws->w1_packed,
                                (size_t) p->k * p->polyw1_packed_bytes);

        MLDSA_SAMPLE_challenge(&ws->cp, ws->ctilde, p->ctilde_bytes, p->tau);
        MLDSA_POLY_ntt(&ws->cp);

        // Compute z = y + INTT(c*s1) one row at a time; pack each z[l] into sig.
        // y[l] is re-sampled (deterministic) instead of being stored.
        {
            uint8_t z_reject = 0U;
            for (uint8_t l = 0U; l < p->l; l++) {
                MLDSA_SAMPLE_gamma1(&ws->t0, ws->rhoprime, (uint16_t) (kappa_base + l), p->gamma1);
                MLDSA_PACK_unpack_polyeta(
                    &ws->t1, &sk_s1[(size_t) l * p->polyeta_packed_bytes], p->eta);
                MLDSA_POLY_ntt(&ws->t1);
                MLDSA_POLY_pointwise_montgomery(&ws->t2, &ws->cp, &ws->t1, 1);
                MLDSA_POLY_invntt_tomont(&ws->t2);
                MLDSA_POLY_add(&ws->t0, &ws->t2);
                MLDSA_POLY_reduce(&ws->t0);

                if (MLDSA_POLY_chknorm(&ws->t0, p->gamma1 - (int32_t) p->beta)) {
                    z_reject = 1U;
                    break;
                }

                MLDSA_PACK_polyz(
                    sig + p->ctilde_bytes + (size_t) l * p->polyz_packed_bytes, &ws->t0, p->gamma1);
            }
            if (z_reject != 0U) {
                continue;
            }
        }

        // For each k: r0 = LowBits(w) - INTT(c*s2); ct0 = INTT(c*t0);
        // hint = MakeHint(ct0 + r0, HighBits(w)). Hints are written straight
        // into the signature as a sorted index list.
        {
            uint8_t     *sig_h   = sig + p->ctilde_bytes + (size_t) p->l * p->polyz_packed_bytes;
            uint32_t     n_hints = 0U;
            unsigned int hints_written = 0U;
            uint8_t      reject        = 0U;

            memset(sig_h, 0, p->polyvech_packed_bytes);

            for (uint8_t k = 0U; k < p->k; k++) {
                // t0 = HighBits(w[k]) = w1[k], t1 = LowBits(w[k]) = w0[k]
                MLDSA_ROUNDING_poly_decompose(&ws->t0, &ws->t1, &ws->w.vec[k], p->gamma2);

                // r0 = w0 - INTT(c*s2[k])  (stored in t1)
                MLDSA_PACK_unpack_polyeta(
                    &ws->t2, &sk_s2[(size_t) k * p->polyeta_packed_bytes], p->eta);
                MLDSA_POLY_ntt(&ws->t2);
                MLDSA_POLY_pointwise_montgomery(&ws->t2, &ws->cp, &ws->t2, 1);
                MLDSA_POLY_invntt_tomont(&ws->t2);
                MLDSA_POLY_sub(&ws->t1, &ws->t2);
                MLDSA_POLY_reduce(&ws->t1);

                // Check ||r0||_inf < gamma2 - beta
                if (MLDSA_POLY_chknorm(&ws->t1, p->gamma2 - (int32_t) p->beta)) {
                    reject = 1U;
                    break;
                }

                // ct0 = INTT(c*t0[k])  (stored in t2)
                MLDSA_PACK_unpack_polyt0(&ws->t2, &sk_t0[(size_t) k * MLDSA_POLYT0_PACKEDBYTES]);
                MLDSA_POLY_ntt(&ws->t2);
                MLDSA_POLY_pointwise_montgomery(&ws->t2, &ws->cp, &ws->t2, 1);
                MLDSA_POLY_invntt_tomont(&ws->t2);
                MLDSA_POLY_reduce(&ws->t2);

                // Check ||ct0||_inf < gamma2
                if (MLDSA_POLY_chknorm(&ws->t2, p->gamma2)) {
                    reject = 1U;
                    break;
                }

                // t2 = ct0 + r0
                MLDSA_POLY_add(&ws->t2, &ws->t1);

                // make_hint per coefficient against w1 (t0)
                for (uint32_t j = 0U; j < MLDSA_N; j++) {
                    uint32_t hbit
                        = MLDSA_ROUNDING_make_hint(ws->t2.coeffs[j], ws->t0.coeffs[j], p->gamma2);
                    if (hbit != 0U) {
                        if (hints_written >= p->omega) {
                            reject = 1U;
                            break;
                        }
                        sig_h[hints_written] = (uint8_t) j;
                        hints_written++;
                    }
                    n_hints += hbit;
                }
                if (reject != 0U) {
                    break;
                }
                sig_h[p->omega + k] = (uint8_t) hints_written;
            }

            if ((reject != 0U) || (n_hints > p->omega)) {
                continue;
            }
        }

        // Success: c_tilde completes the signature (z and h already packed).
        memcpy(sig, ws->ctilde, p->ctilde_bytes);
        *sig_actual_len = p->sig_bytes;
        error           = CX_OK;
        goto cleanup;
    }

    // Exhausted attempts
    error = CX_INTERNAL_ERROR;

cleanup:
    explicit_bzero(zero_rnd, sizeof(zero_rnd));
    explicit_bzero(ws, sizeof(*ws));

    return error;
}

/**
 * @brief Core ML-DSA verification routine (FIPS 204, Algorithms 3 & 8).
 *
 * Verifies an ML-DSA signature.  Accepts either a formatted message
 * @p formatted_mprime (from which mu is derived) or a pre-computed
 * @p precomputed_mu; exactly one must be non-NULL.
 *
 * @param[in]  sig               Signature to verify.
 * @param[in]  sig_len           Signature length in bytes.
 * @param[in]  formatted_mprime  Formatted message M' (NULL when using @p precomputed_mu).
 * @param[in]  precomputed_mu    Pre-computed mu (NULL when using @p formatted_mprime).
 * @param[in]  pk                Public key.
 * @param[in]  pk_len            Public key length in bytes.
 * @param[in]  param             ML-DSA parameter set selector.
 *
 * @return                       CX_OK if the signature is valid, error code otherwise.
 */
cx_err_t MLDSA_internal_verify_core(const uint8_t                   *sig,
                                    size_t                           sig_len,
                                    const MLDSA_formatted_message_t *formatted_mprime,
                                    const uint8_t                   *precomputed_mu,
                                    const uint8_t                   *pk,
                                    size_t                           pk_len,
                                    MLDSA_param_t                    param)
{
    MLDSA_verify_stack_workspace_t  ws_local = {0};
    MLDSA_verify_stack_workspace_t *ws       = &ws_local;
    const MLDSA_param_info_t       *p        = NULL;
    const uint8_t                  *sig_z    = NULL;
    const uint8_t                  *sig_h    = NULL;
    uint32_t                        k_offset = 0U;
    cx_err_t                        error    = CX_INTERNAL_ERROR;

    if ((sig == NULL) || (pk == NULL)) {
        error = CX_INVALID_PARAMETER;
        goto cleanup;
    }
    if ((formatted_mprime == NULL) && (precomputed_mu == NULL)) {
        error = CX_INVALID_PARAMETER;
        goto cleanup;
    }

    if (param >= MLDSA_NUM_PARAM_SETS) {
        error = CX_INVALID_PARAMETER_VALUE;
        goto cleanup;
    }

    p = &MLDSA_PARAM[param];

    if (pk_len < p->pk_bytes) {
        error = CX_INVALID_PARAMETER_SIZE;
        goto cleanup;
    }
    if (sig_len < p->sig_bytes) {
        error = CX_INVALID_PARAMETER_SIZE;
        goto cleanup;
    }

    explicit_bzero(ws, sizeof(*ws));

    memcpy(ws->rho, pk, MLDSA_SEEDBYTES);
    memcpy(ws->ctilde, sig, p->ctilde_bytes);

    sig_z = &sig[p->ctilde_bytes];
    sig_h = &sig_z[(size_t) p->l * p->polyz_packed_bytes];

    // Validate hint encoding without materializing the full h vector.
    for (uint32_t i = 0U; i < p->k; i++) {
        uint32_t limit = (uint32_t) sig_h[p->omega + i];
        if ((limit < k_offset) || (limit > p->omega)) {
            error = CX_INVALID_PARAMETER;
            goto cleanup;
        }

        for (uint32_t j = k_offset; j < limit; j++) {
            if ((j > k_offset) && (sig_h[j] <= sig_h[j - 1U])) {
                error = CX_INVALID_PARAMETER;
                goto cleanup;
            }
        }
        k_offset = limit;
    }

    for (uint32_t j = k_offset; j < p->omega; j++) {
        if (sig_h[j] != 0U) {
            error = CX_INVALID_PARAMETER;
            goto cleanup;
        }
    }

    // Stream z and check ||z||_inf < gamma1 - beta.
    for (uint8_t j = 0U; j < p->l; j++) {
        MLDSA_PACK_unpack_polyz(&ws->ztmp, &sig_z[(size_t) j * p->polyz_packed_bytes], p->gamma1);
        if (MLDSA_POLY_chknorm(&ws->ztmp, p->gamma1 - (int32_t) p->beta)) {
            error = CX_INVALID_PARAMETER;
            goto cleanup;
        }
    }

    // Compute or import mu
    if (precomputed_mu != NULL) {
        memcpy(ws->mu, precomputed_mu, MLDSA_CRHBYTES);
    }
    else {
        MLDSA_UTIL_shake256(ws->overlay.setup_phase.tr, MLDSA_TRBYTES, pk, p->pk_bytes);
        mldsa_compute_mu(ws->mu, ws->overlay.setup_phase.tr, formatted_mprime);
    }

    // Sample challenge c from ctilde
    MLDSA_SAMPLE_challenge(&ws->cp, ws->ctilde, p->ctilde_bytes, p->tau);
    MLDSA_POLY_ntt(&ws->cp);

    // Fused streaming verify: materialize one z, one t1 and one hint row at a time.
    k_offset = 0U;
    for (uint8_t i = 0U; i < p->k; i++) {
        uint32_t limit = (uint32_t) sig_h[p->omega + i];

        for (uint8_t j = 0U; j < p->l; j++) {
            uint16_t nonce = ((uint16_t) i << 8U) | (uint16_t) j;
            MLDSA_PACK_unpack_polyz(
                &ws->ztmp, &sig_z[(size_t) j * p->polyz_packed_bytes], p->gamma1);
            MLDSA_POLY_ntt(&ws->ztmp);
            MLDSA_SAMPLE_uniform(&ws->overlay.az_phase.aij, ws->rho, nonce);
            MLDSA_POLY_pointwise_montgomery(
                &ws->overlay.az_phase.dot, &ws->overlay.az_phase.aij, &ws->ztmp, (j == 0U) ? 1 : 0);
        }

        MLDSA_PACK_unpack_polyt1(&ws->t1tmp,
                                 &pk[MLDSA_SEEDBYTES + (size_t) i * MLDSA_POLYT1_PACKEDBYTES]);
        MLDSA_POLY_shiftl(&ws->t1tmp);
        MLDSA_POLY_ntt(&ws->t1tmp);
        MLDSA_POLY_pointwise_montgomery(&ws->t1tmp, &ws->cp, &ws->t1tmp, 1);

        MLDSA_POLY_sub(&ws->overlay.az_phase.dot, &ws->t1tmp);
        MLDSA_POLY_reduce(&ws->overlay.az_phase.dot);
        MLDSA_POLY_invntt_tomont(&ws->overlay.az_phase.dot);
        MLDSA_POLY_caddq_all(&ws->overlay.az_phase.dot);

        explicit_bzero(&ws->htmp, sizeof(ws->htmp));
        for (uint32_t j = k_offset; j < limit; j++) {
            ws->htmp.coeffs[sig_h[j]] = 1;
        }
        k_offset = limit;

        MLDSA_ROUNDING_poly_use_hint(&ws->t1tmp, &ws->overlay.az_phase.dot, &ws->htmp, p->gamma2);

        MLDSA_PACK_polyw1(
            &ws->w1_packed[(size_t) i * p->polyw1_packed_bytes], &ws->t1tmp, p->gamma2);
    }

    MLDSA_UTIL_shake256_two(ws->ctilde2,
                            p->ctilde_bytes,
                            ws->mu,
                            MLDSA_CRHBYTES,
                            ws->w1_packed,
                            (size_t) p->k * p->polyw1_packed_bytes);

    // Compare c_tilde
    if (memcmp(ws->ctilde, ws->ctilde2, p->ctilde_bytes) != 0) {
        error = CX_INVALID_PARAMETER;
        goto cleanup;
    }

    error = CX_OK;

cleanup:
    explicit_bzero(ws, sizeof(*ws));
    return error;
}

#else /* HAVE_MLDSA_OPTIMIZATION */

/*===========================================================================
 * OPTIMIZED LOW-RAM IMPLEMENTATION
 *
 * Key techniques applied:
 * 1. smallpoly (int16_t) for c*s1, c*s2 via small NTT mod 3329
 * 2. Schoolbook c*t0 / c*t1 directly from packed secret/public key
 * 3. Compressed challenge (68 bytes instead of 1024 byte poly)
 * 4. Compressed w buffers (768 bytes/row instead of 1024 bytes)
 * 5. Fused A expansion (streaming 3 bytes at a time into compressed w)
 * 6. Streaming gamma1 sampling (5-9 byte buffer)
 * 7. Hint index list in verify (max 80 bytes vs 1024 byte poly)
 * 8. Eliminated full y/yhat vectors
 *===========================================================================*/

/**
 * @brief Stack-allocated workspace for optimized #MLDSA_internal_sign_core.
 */
typedef struct MLDSA_sign_opt_workspace_s {
    uint8_t rho[MLDSA_SEEDBYTES];                  /**< Public seed rho.                  */
    uint8_t K[MLDSA_SEEDBYTES];                    /**< Secret seed K.                    */
    uint8_t tr[MLDSA_TRBYTES];                     /**< tr = H(pk).                       */
    uint8_t mu[MLDSA_CRHBYTES];                    /**< Message representative.           */
    uint8_t rhoprime[MLDSA_CRHBYTES];              /**< Derived nonce seed.               */
    uint8_t ctilde[64U];                           /**< Challenge hash.                   */
    uint8_t ccomp[MLDSA_CCOMP_BYTES];              /**< Compressed challenge.             */
    uint8_t wcomp[MLDSA_MAX_K][MLDSA_WCOMP_BYTES]; /**< Compressed w (768 B/row). */
    union {
        mldsa_poly full; /**< Full-size polynomial temporary.   */
        struct {
            mldsa_smallpoly scp;  /**< Small NTT challenge.              */
            mldsa_smallpoly stmp; /**< Small NTT secret poly.            */
        } small;
    } polybuf;
    uint8_t w1_packed[MLDSA_MAX_K * 192U]; /**< Packed w1 for hashing.         */
} MLDSA_sign_opt_workspace_t;

/**
 * @brief Core ML-DSA signing (optimized low-RAM version).
 */
cx_err_t MLDSA_internal_sign_core(uint8_t                         *sig,
                                  size_t                           sig_len,
                                  size_t                          *sig_actual_len,
                                  const MLDSA_formatted_message_t *formatted_mprime,
                                  const uint8_t                   *precomputed_mu,
                                  uint8_t                         *rnd,
                                  size_t                           rnd_len,
                                  const uint8_t                   *sk,
                                  size_t                           sk_len,
                                  MLDSA_param_t                    param)
{
    MLDSA_sign_opt_workspace_t  ws_local                 = {0};
    MLDSA_sign_opt_workspace_t *ws                       = &ws_local;
    const MLDSA_param_info_t   *p                        = NULL;
    const uint8_t              *rnd_input                = rnd;
    uint8_t                     zero_rnd[MLDSA_RNDBYTES] = {0};
    cx_err_t                    error                    = CX_INTERNAL_ERROR;
    uint16_t                    kappa                    = 0U;
    uint32_t                    attempts                 = 0U;

    if ((sig == NULL) || (sk == NULL) || (sig_actual_len == NULL)) {
        error = CX_INVALID_PARAMETER;
        goto cleanup;
    }
    if ((formatted_mprime == NULL) && (precomputed_mu == NULL)) {
        error = CX_INVALID_PARAMETER;
        goto cleanup;
    }
    if ((rnd != NULL) && (rnd_len != MLDSA_RNDBYTES)) {
        error = CX_INVALID_PARAMETER;
        goto cleanup;
    }
    if ((rnd == NULL) && (rnd_len != 0U)) {
        error = CX_INVALID_PARAMETER;
        goto cleanup;
    }
    if (param >= MLDSA_NUM_PARAM_SETS) {
        error = CX_INVALID_PARAMETER_VALUE;
        goto cleanup;
    }

    p = &MLDSA_PARAM[param];

    if (sk_len < p->sk_bytes) {
        error = CX_INVALID_PARAMETER_SIZE;
        goto cleanup;
    }
    if (sig_len < p->sig_bytes) {
        error = CX_INVALID_PARAMETER_SIZE;
        goto cleanup;
    }

    if (rnd_input == NULL) {
        rnd_input = zero_rnd;
    }

    explicit_bzero(ws, sizeof(*ws));

    // Unpack secret-key header
    memcpy(ws->rho, sk, MLDSA_SEEDBYTES);
    memcpy(ws->K, &sk[MLDSA_SEEDBYTES], MLDSA_SEEDBYTES);
    memcpy(ws->tr, &sk[2U * MLDSA_SEEDBYTES], MLDSA_TRBYTES);

    const uint8_t *sk_s1 = &sk[2U * MLDSA_SEEDBYTES + MLDSA_TRBYTES];
    const uint8_t *sk_s2 = &sk_s1[(size_t) p->l * p->polyeta_packed_bytes];
    const uint8_t *sk_t0 = &sk_s2[(size_t) p->k * p->polyeta_packed_bytes];

    // Compute or import mu
    if (precomputed_mu != NULL) {
        memcpy(ws->mu, precomputed_mu, MLDSA_CRHBYTES);
    }
    else {
        mldsa_compute_mu(ws->mu, ws->tr, formatted_mprime);
    }

    // Compute rhoprime = H(K || rnd || mu, 64)
    {
        cx_sha3_t sha3_ctx = {0};
        cx_shake256_init_no_throw(&sha3_ctx, MLDSA_CRHBYTES * 8U);
        cx_hash_no_throw((cx_hash_t *) &sha3_ctx, 0, ws->K, MLDSA_SEEDBYTES, NULL, 0);
        cx_hash_no_throw((cx_hash_t *) &sha3_ctx, 0, rnd_input, MLDSA_RNDBYTES, NULL, 0);
        cx_hash_no_throw(
            (cx_hash_t *) &sha3_ctx, CX_LAST, ws->mu, MLDSA_CRHBYTES, ws->rhoprime, MLDSA_CRHBYTES);
    }

    // Rejection sampling loop
    while (attempts < MLDSA_MAX_SIGN_ATTEMPTS) {
        attempts++;

        // Zero compressed w buffers
        for (uint8_t k_idx = 0U; k_idx < p->k; k_idx++) {
            memset(ws->wcomp[k_idx], 0, MLDSA_WCOMP_BYTES);
        }

        // For each y polynomial: sample, NTT, fuse A expansion into wcomp
        for (uint8_t l_idx = 0U; l_idx < p->l; l_idx++) {
            MLDSA_LOWRAM_sample_gamma1(
                &ws->polybuf.full, ws->rhoprime, (uint16_t) (kappa + l_idx), p->gamma1);
            MLDSA_POLY_ntt(&ws->polybuf.full);

            for (uint8_t k_idx = 0U; k_idx < p->k; k_idx++) {
                uint16_t nonce = ((uint16_t) k_idx << 8U) | (uint16_t) l_idx;
                MLDSA_LOWRAM_expand_aij_accum(ws->wcomp[k_idx], &ws->polybuf.full, ws->rho, nonce);
            }
        }
        kappa += (uint16_t) p->l;

        // For each row: unpack wcomp, INTT, repack as reduced, compute highbits, pack w1
        for (uint8_t k_idx = 0U; k_idx < p->k; k_idx++) {
            MLDSA_LOWRAM_polyw_unpack(&ws->polybuf.full, ws->wcomp[k_idx]);
            MLDSA_POLY_invntt_tomont(&ws->polybuf.full);
            MLDSA_POLY_caddq_all(&ws->polybuf.full);
            MLDSA_LOWRAM_polyw_pack(ws->wcomp[k_idx], &ws->polybuf.full);
            MLDSA_LOWRAM_poly_highbits(&ws->polybuf.full, &ws->polybuf.full, p->gamma2);
            MLDSA_PACK_polyw1(&ws->w1_packed[(size_t) k_idx * p->polyw1_packed_bytes],
                              &ws->polybuf.full,
                              p->gamma2);
        }

        // Compute challenge hash ctilde
        MLDSA_UTIL_shake256_two(ws->ctilde,
                                p->ctilde_bytes,
                                ws->mu,
                                MLDSA_CRHBYTES,
                                ws->w1_packed,
                                (size_t) p->k * p->polyw1_packed_bytes);

        MLDSA_SAMPLE_challenge(&ws->polybuf.full, ws->ctilde, p->ctilde_bytes, p->tau);
        MLDSA_LOWRAM_challenge_compress(ws->ccomp, &ws->polybuf.full, p->tau);

        // Convert challenge to small NTT form for c*s multiplications
        MLDSA_SMALLPOLY_ntt_copy(&ws->polybuf.small.scp, &ws->polybuf.full);

        // Compute z = y + c*s1 for each l, check norm, and pack into sig
        {
            uint8_t z_reject = 0U;
            for (uint8_t l_idx = 0U; l_idx < p->l; l_idx++) {
                // Need challenge in small NTT form: re-decompress
                if (l_idx != 0U) {
                    MLDSA_LOWRAM_challenge_decompress(&ws->polybuf.full, ws->ccomp, p->tau);
                    MLDSA_SMALLPOLY_ntt_copy(&ws->polybuf.small.scp, &ws->polybuf.full);
                }

                // Unpack s1[l_idx] into small poly
                MLDSA_SMALLPOLY_unpack_eta(&ws->polybuf.small.stmp,
                                           &sk_s1[(size_t) l_idx * p->polyeta_packed_bytes],
                                           p->eta);
                MLDSA_SMALLPOLY_ntt(ws->polybuf.small.stmp.coeffs);

                // Compute c*s1[l_idx] via small basemul + INTT (full poly)
                MLDSA_SMALLPOLY_basemul_invntt(
                    &ws->polybuf.full, &ws->polybuf.small.scp, &ws->polybuf.small.stmp);

                // z[l_idx] = y[l_idx] + c*s1[l_idx] (re-sample y and add)
                MLDSA_LOWRAM_sample_gamma1_add(&ws->polybuf.full,
                                               &ws->polybuf.full,
                                               ws->rhoprime,
                                               (uint16_t) (kappa - (uint16_t) p->l + l_idx),
                                               p->gamma1);
                MLDSA_POLY_reduce(&ws->polybuf.full);

                // Check ||z||_inf < gamma1 - beta
                if (MLDSA_POLY_chknorm(&ws->polybuf.full, p->gamma1 - (int32_t) p->beta)) {
                    z_reject = 1U;
                    break;
                }

                // Pack z into signature
                MLDSA_PACK_polyz(sig + p->ctilde_bytes + (size_t) l_idx * p->polyz_packed_bytes,
                                 &ws->polybuf.full,
                                 p->gamma1);
            }
            if (z_reject != 0U) {
                continue;
            }
        }

        // Compute r0 = w - c*s2, check norm
        {
            uint8_t r0_reject = 0U;
            for (uint8_t k_idx = 0U; k_idx < p->k; k_idx++) {
                // Re-decompress challenge and prepare small NTT
                MLDSA_LOWRAM_challenge_decompress(&ws->polybuf.full, ws->ccomp, p->tau);
                MLDSA_SMALLPOLY_ntt_copy(&ws->polybuf.small.scp, &ws->polybuf.full);

                // Unpack s2[k_idx] into small poly
                MLDSA_SMALLPOLY_unpack_eta(&ws->polybuf.small.stmp,
                                           &sk_s2[(size_t) k_idx * p->polyeta_packed_bytes],
                                           p->eta);
                MLDSA_SMALLPOLY_ntt(ws->polybuf.small.stmp.coeffs);

                // c*s2[k_idx] via small basemul + INTT
                MLDSA_SMALLPOLY_basemul_invntt(
                    &ws->polybuf.full, &ws->polybuf.small.scp, &ws->polybuf.small.stmp);

                // r0 = wcomp - c*s2: subtract from compressed w
                MLDSA_LOWRAM_polyw_sub(&ws->polybuf.full, ws->wcomp[k_idx], &ws->polybuf.full);
                MLDSA_POLY_reduce(&ws->polybuf.full);

                // Store back into wcomp for hint computation
                MLDSA_LOWRAM_polyw_pack(ws->wcomp[k_idx], &ws->polybuf.full);

                // r0 = LowBits(w) - c*s2: reconstruct
                // from the stored (w - c*s2) and w1 = HighBits(w).
                MLDSA_LOWRAM_poly_r0(&ws->polybuf.full,
                                     ws->wcomp[k_idx],
                                     &ws->w1_packed[(size_t) k_idx * p->polyw1_packed_bytes],
                                     p->gamma2);

                if (MLDSA_POLY_chknorm(&ws->polybuf.full, p->gamma2 - (int32_t) p->beta)) {
                    r0_reject = 1U;
                    break;
                }
            }
            if (r0_reject != 0U) {
                continue;
            }
        }

        // Fused ct0 computation, norm check, and hint generation
        {
            uint32_t     n_hints       = 0U;
            uint8_t      reject        = 0U;
            unsigned int hints_written = 0U;

            // Zero hint portion of signature
            memset(sig + p->ctilde_bytes + (size_t) p->l * p->polyz_packed_bytes,
                   0,
                   p->polyvech_packed_bytes);

            for (uint8_t k_idx = 0U; k_idx < p->k; k_idx++) {
                MLDSA_LOWRAM_schoolbook_t0(&ws->polybuf.full,
                                           ws->ccomp,
                                           &sk_t0[(size_t) k_idx * MLDSA_POLYT0_PACKEDBYTES],
                                           p->tau);
                MLDSA_POLY_reduce(&ws->polybuf.full);

                // Check ||ct0||_inf < gamma2
                if (MLDSA_POLY_chknorm(&ws->polybuf.full, p->gamma2)) {
                    reject = 1U;
                    break;
                }

                // make_hint from ct0 and (w - cs2) stored in wcomp
                uint32_t row_hints = MLDSA_LOWRAM_make_hint(
                    &ws->polybuf.full,
                    &ws->polybuf.full,
                    ws->wcomp[k_idx],
                    &ws->w1_packed[(size_t) k_idx * p->polyw1_packed_bytes],
                    p->gamma2);
                n_hints += row_hints;

                if (n_hints > p->omega) {
                    reject = 1U;
                    break;
                }

                // Pack hint into signature
                {
                    uint8_t *sig_h = sig + p->ctilde_bytes + (size_t) p->l * p->polyz_packed_bytes;
                    for (uint32_t j = 0U; j < MLDSA_N; j++) {
                        if (ws->polybuf.full.coeffs[j] != 0) {
                            sig_h[hints_written] = (uint8_t) j;
                            hints_written++;
                        }
                    }
                    sig_h[p->omega + k_idx] = (uint8_t) hints_written;
                }
            }

            if (reject != 0U) {
                continue;
            }

            // Pad remaining hint bytes with zeros
            {
                uint8_t *sig_h = sig + p->ctilde_bytes + (size_t) p->l * p->polyz_packed_bytes;
                while (hints_written < p->omega) {
                    sig_h[hints_written] = 0U;
                    hints_written++;
                }
            }
        }

        // Success: pack ctilde into signature
        memcpy(sig, ws->ctilde, p->ctilde_bytes);
        *sig_actual_len = p->sig_bytes;
        error           = CX_OK;
        goto cleanup;
    }

    // Exhausted attempts
    error = CX_INTERNAL_ERROR;

cleanup:
    explicit_bzero(zero_rnd, sizeof(zero_rnd));
    explicit_bzero(ws, sizeof(*ws));
    return error;
}

/**
 * @brief Stack-allocated workspace for optimized #MLDSA_internal_verify_core.
 */
typedef struct MLDSA_verify_opt_workspace_s {
    uint8_t    rho[MLDSA_SEEDBYTES];          /**< Public seed rho.                 */
    uint8_t    ctilde[64U];                   /**< Challenge hash from signature.   */
    uint8_t    mu[MLDSA_CRHBYTES];            /**< Message representative.          */
    uint8_t    ccomp[MLDSA_CCOMP_BYTES];      /**< Compressed challenge.            */
    uint8_t    wcomp[MLDSA_WCOMP_BYTES];      /**< Compressed w for one row.        */
    uint8_t    w1_packed[MLDSA_MAX_K * 192U]; /**< Packed reconstructed w1'.      */
    uint8_t    ctilde2[64U];                  /**< Recomputed challenge hash.       */
    uint8_t    h_indices[MLDSA_MAX_OMEGA];    /**< Hint indices for current row.    */
    mldsa_poly tmp;                           /**< General-purpose poly temporary.  */
    mldsa_poly ct1;                           /**< c*t1 temporary for schoolbook.   */
    union {
        uint8_t tr[MLDSA_TRBYTES]; /**< tr for computing mu.             */
    } early;
} MLDSA_verify_opt_workspace_t;

/**
 * @brief Core ML-DSA verification (optimized low-RAM version).
 */
cx_err_t MLDSA_internal_verify_core(const uint8_t                   *sig,
                                    size_t                           sig_len,
                                    const MLDSA_formatted_message_t *formatted_mprime,
                                    const uint8_t                   *precomputed_mu,
                                    const uint8_t                   *pk,
                                    size_t                           pk_len,
                                    MLDSA_param_t                    param)
{
    MLDSA_verify_opt_workspace_t  ws_local = {0};
    MLDSA_verify_opt_workspace_t *ws       = &ws_local;
    const MLDSA_param_info_t     *p        = NULL;
    const uint8_t                *sig_z    = NULL;
    const uint8_t                *sig_h    = NULL;
    uint32_t                      k_offset = 0U;
    cx_err_t                      error    = CX_INTERNAL_ERROR;

    if ((sig == NULL) || (pk == NULL)) {
        error = CX_INVALID_PARAMETER;
        goto cleanup;
    }
    if ((formatted_mprime == NULL) && (precomputed_mu == NULL)) {
        error = CX_INVALID_PARAMETER;
        goto cleanup;
    }
    if (param >= MLDSA_NUM_PARAM_SETS) {
        error = CX_INVALID_PARAMETER_VALUE;
        goto cleanup;
    }

    p = &MLDSA_PARAM[param];

    if (pk_len < p->pk_bytes) {
        error = CX_INVALID_PARAMETER_SIZE;
        goto cleanup;
    }
    if (sig_len < p->sig_bytes) {
        error = CX_INVALID_PARAMETER_SIZE;
        goto cleanup;
    }

    explicit_bzero(ws, sizeof(*ws));

    memcpy(ws->rho, pk, MLDSA_SEEDBYTES);
    memcpy(ws->ctilde, sig, p->ctilde_bytes);

    sig_z = &sig[p->ctilde_bytes];
    sig_h = &sig_z[(size_t) p->l * p->polyz_packed_bytes];

    // Validate hint encoding
    for (uint32_t i = 0U; i < p->k; i++) {
        uint32_t limit = (uint32_t) sig_h[p->omega + i];
        if ((limit < k_offset) || (limit > p->omega)) {
            error = CX_INVALID_PARAMETER;
            goto cleanup;
        }
        for (uint32_t j = k_offset; j < limit; j++) {
            if ((j > k_offset) && (sig_h[j] <= sig_h[j - 1U])) {
                error = CX_INVALID_PARAMETER;
                goto cleanup;
            }
        }
        k_offset = limit;
    }
    for (uint32_t j = k_offset; j < p->omega; j++) {
        if (sig_h[j] != 0U) {
            error = CX_INVALID_PARAMETER;
            goto cleanup;
        }
    }

    // Stream z and check ||z||_inf < gamma1 - beta
    for (uint8_t j = 0U; j < p->l; j++) {
        MLDSA_PACK_unpack_polyz(&ws->tmp, &sig_z[(size_t) j * p->polyz_packed_bytes], p->gamma1);
        if (MLDSA_POLY_chknorm(&ws->tmp, p->gamma1 - (int32_t) p->beta)) {
            error = CX_INVALID_PARAMETER;
            goto cleanup;
        }
    }

    // Compute or import mu
    if (precomputed_mu != NULL) {
        memcpy(ws->mu, precomputed_mu, MLDSA_CRHBYTES);
    }
    else {
        MLDSA_UTIL_shake256(ws->early.tr, MLDSA_TRBYTES, pk, p->pk_bytes);
        mldsa_compute_mu(ws->mu, ws->early.tr, formatted_mprime);
    }

    // Compress challenge for schoolbook use
    MLDSA_SAMPLE_challenge(&ws->tmp, ws->ctilde, p->ctilde_bytes, p->tau);
    MLDSA_LOWRAM_challenge_compress(ws->ccomp, &ws->tmp, p->tau);

    // Fused streaming verify: one row at a time
    k_offset = 0U;
    for (uint8_t i = 0U; i < p->k; i++) {
        uint32_t limit = (uint32_t) sig_h[p->omega + i];

        // Compute Az[i] = sum_j A[i][j] * NTT(z[j]) using fused expansion
        memset(ws->wcomp, 0, MLDSA_WCOMP_BYTES);
        for (uint8_t j = 0U; j < p->l; j++) {
            uint16_t nonce = ((uint16_t) i << 8U) | (uint16_t) j;
            MLDSA_PACK_unpack_polyz(
                &ws->tmp, &sig_z[(size_t) j * p->polyz_packed_bytes], p->gamma1);
            MLDSA_POLY_ntt(&ws->tmp);
            MLDSA_LOWRAM_expand_aij_accum(ws->wcomp, &ws->tmp, ws->rho, nonce);
        }

        // Unpack Az, INTT
        MLDSA_LOWRAM_polyw_unpack(&ws->tmp, ws->wcomp);
        MLDSA_POLY_reduce(&ws->tmp);
        MLDSA_POLY_invntt_tomont(&ws->tmp);

        // Subtract c*t1*2^d using schoolbook from packed pk
        MLDSA_LOWRAM_schoolbook_t1(&ws->ct1,
                                   ws->ccomp,
                                   &pk[MLDSA_SEEDBYTES + (size_t) i * MLDSA_POLYT1_PACKEDBYTES],
                                   p->tau);
        MLDSA_POLY_sub(&ws->tmp, &ws->ct1);
        MLDSA_POLY_reduce(&ws->tmp);
        MLDSA_POLY_caddq_all(&ws->tmp);

        // Extract hint indices for this row
        uint32_t num_hints = limit - k_offset;
        for (uint32_t j = 0U; j < num_hints; j++) {
            ws->h_indices[j] = sig_h[k_offset + j];
        }
        k_offset = limit;

        // Use hint (index-list form) to reconstruct w1'
        MLDSA_LOWRAM_use_hint_indices(&ws->tmp, &ws->tmp, ws->h_indices, num_hints, p->gamma2);

        // Pack w1 row
        MLDSA_PACK_polyw1(&ws->w1_packed[(size_t) i * p->polyw1_packed_bytes], &ws->tmp, p->gamma2);
    }

    // Recompute challenge hash
    MLDSA_UTIL_shake256_two(ws->ctilde2,
                            p->ctilde_bytes,
                            ws->mu,
                            MLDSA_CRHBYTES,
                            ws->w1_packed,
                            (size_t) p->k * p->polyw1_packed_bytes);

    // Compare c_tilde
    if (memcmp(ws->ctilde, ws->ctilde2, p->ctilde_bytes) != 0) {
        error = CX_INVALID_PARAMETER;
        goto cleanup;
    }

    error = CX_OK;

cleanup:
    explicit_bzero(ws, sizeof(*ws));
    return error;
}

#endif /* HAVE_MLDSA_OPTIMIZATION */

/*********************
 *  GLOBAL FUNCTIONS
 *********************/

/*---------------------------------------------------------------------------
 * KeyGen (FIPS 204, Algorithm 1)
 *---------------------------------------------------------------------------*/
cx_err_t MLDSA_keygen(uint8_t *pk, size_t pk_len, uint8_t *sk, size_t sk_len, MLDSA_param_t param)
{
    cx_err_t error                 = CX_INTERNAL_ERROR;
    uint8_t  seed[MLDSA_SEEDBYTES] = {0};

    if ((pk == NULL) || (sk == NULL)) {
        error = CX_INVALID_PARAMETER;
        goto cleanup;
    }

    if (param >= MLDSA_NUM_PARAM_SETS) {
        error = CX_INVALID_PARAMETER_VALUE;
        goto cleanup;
    }

    cx_rng_no_throw(seed, MLDSA_SEEDBYTES);
    error = MLDSA_internal_keygen(pk, pk_len, sk, sk_len, seed, param);

cleanup:
    explicit_bzero(seed, sizeof(seed));
    return error;
}

/*---------------------------------------------------------------------------
 * Sign (FIPS 204, Algorithms 2 & 7)
 *---------------------------------------------------------------------------*/
cx_err_t MLDSA_sign(uint8_t       *sig,
                    size_t         sig_len,
                    size_t        *sig_actual_len,
                    const uint8_t *msg,
                    size_t         msg_len,
                    const uint8_t *ctx,
                    size_t         ctx_len,
                    const uint8_t *sk,
                    size_t         sk_len,
                    MLDSA_param_t  param)
{
    MLDSA_formatted_message_t mprime              = {0};
    uint8_t                   rnd[MLDSA_RNDBYTES] = {0};
    cx_err_t error = mldsa_format_message_pure(&mprime, ctx, ctx_len, msg, msg_len);

    if (error != CX_OK) {
        goto cleanup;
    }

    cx_rng_no_throw(rnd, MLDSA_RNDBYTES);
    error = MLDSA_internal_sign_core(
        sig, sig_len, sig_actual_len, &mprime, NULL, rnd, sizeof(rnd), sk, sk_len, param);

cleanup:
    explicit_bzero(rnd, sizeof(rnd));
    return error;
}

/*---------------------------------------------------------------------------
 * Verify (FIPS 204, Algorithms 3 & 8)
 *---------------------------------------------------------------------------*/
cx_err_t MLDSA_verify(const uint8_t *sig,
                      size_t         sig_len,
                      const uint8_t *msg,
                      size_t         msg_len,
                      const uint8_t *ctx,
                      size_t         ctx_len,
                      const uint8_t *pk,
                      size_t         pk_len,
                      MLDSA_param_t  param)
{
    MLDSA_formatted_message_t mprime = {0};
    cx_err_t error = mldsa_format_message_pure(&mprime, ctx, ctx_len, msg, msg_len);

    if (error != CX_OK) {
        return error;
    }

    return MLDSA_internal_verify_core(sig, sig_len, &mprime, NULL, pk, pk_len, param);
}

/*---------------------------------------------------------------------------
 * HashML-DSA Sign (FIPS 204, Algorithm 4)
 *---------------------------------------------------------------------------*/
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
                            MLDSA_param_t   param)
{
    MLDSA_formatted_message_t mprime              = {0};
    uint8_t                   rnd[MLDSA_RNDBYTES] = {0};
    cx_err_t error = mldsa_format_message_prehash(&mprime, ctx, ctx_len, prehash_alg, ph, ph_len);

    if (error != CX_OK) {
        goto cleanup;
    }

    cx_rng_no_throw(rnd, MLDSA_RNDBYTES);
    error = MLDSA_internal_sign_core(
        sig, sig_len, sig_actual_len, &mprime, NULL, rnd, sizeof(rnd), sk, sk_len, param);

cleanup:
    explicit_bzero(rnd, sizeof(rnd));
    return error;
}

/*---------------------------------------------------------------------------
 * HashML-DSA Verify (FIPS 204, Algorithm 5)
 *---------------------------------------------------------------------------*/
cx_err_t MLDSA_verify_prehash(const uint8_t  *sig,
                              size_t          sig_len,
                              const uint8_t  *ph,
                              size_t          ph_len,
                              const uint8_t  *ctx,
                              size_t          ctx_len,
                              const uint8_t  *pk,
                              size_t          pk_len,
                              MLDSA_prehash_t prehash_alg,
                              MLDSA_param_t   param)
{
    MLDSA_formatted_message_t mprime = {0};
    cx_err_t error = mldsa_format_message_prehash(&mprime, ctx, ctx_len, prehash_alg, ph, ph_len);

    if (error != CX_OK) {
        return error;
    }

    return MLDSA_internal_verify_core(sig, sig_len, &mprime, NULL, pk, pk_len, param);
}
