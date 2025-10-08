
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
 * @file    lcx_rng.h
 * @brief   Random Number Generation
 *
 * Random numbers with different sizes can be generated: a 8-bit random number, a 32-bit random
 * number or a random number of arbitrary size. In this case, the number is returned as a buffer of
 * random bytes. The random number can also be generated within a specific range.
 */

#ifndef LCX_RNG_H
#define LCX_RNG_H

#include "lcx_wrappers.h"
#include "lcx_hash.h"
#include "libcxng.h"

#ifdef HAVE_RNG_RFC6979
#define CX_RFC6979_BUFFER_LENGTH 64
#define CX_RFC6979_MAX_RLEN      66

typedef struct {
    uint8_t  v[CX_RFC6979_BUFFER_LENGTH + 1];
    uint8_t  k[CX_RFC6979_BUFFER_LENGTH];
    uint8_t  q[CX_RFC6979_MAX_RLEN];
    uint32_t q_len;
    uint32_t r_len;
    uint8_t  tmp[CX_RFC6979_MAX_RLEN];
    cx_md_t  hash_id;
    size_t   md_len;

    union {
#if (!defined(HAVE_SHA512) && !defined(HAVE_SHA384) && !defined(HAVE_SHA256) \
     && !defined(HAVE_SHA224))                                               \
    || !defined(HAVE_HMAC)
#error No hmac defined for rfc6979 support
#endif

        cx_hmac_t hmac;

#if defined(HAVE_SHA512) || defined(HAVE_SHA384)
        cx_hmac_sha512_t hmac_sha512;
#endif

#if defined(HAVE_SHA256) || defined(HAVE_SHA224)
        cx_hmac_sha256_t hmac_sha256;
#endif
    };
} cx_rnd_rfc6979_ctx_t;
#endif  // HAVE_RNG_RFC6979

#ifdef HAVE_RNG
/**
 * @brief   Generates a random buffer such that
 *          each byte is between 0 and 255.
 *
 * @param[out] buffer Buffer to hold the random data.
 *
 * @param[in]  len    Length of the buffer i.e. number of
 *                    random bytes to put into the buffer.
 */
void cx_rng_no_throw(uint8_t *buffer, size_t len);

/**
 * @brief   Generates a random buffer such that
 *          each byte is between 0 and 255.
 *
 * @param[out] buffer Buffer to hold the random data.
 *
 * @param[in]  len    Length of the buffer i.e. number of
 *                    random bytes to put into the buffer.
 *
 * @return            Pointer to the buffer.
 */
static inline unsigned char *cx_rng(uint8_t *buffer, size_t len)
{
    cx_rng_no_throw(buffer, len);
    return buffer;
}

/**
 * @brief   Returns 32 random bits.
 *
 * @return  A 32-bit random number.
 */
static inline uint32_t cx_rng_u32(void)
{
    uint32_t r;
    cx_rng_no_throw((uint8_t *) &r, sizeof(uint32_t));
    return r;
}

/**
 * @brief   Returns 8 random bits.
 *
 * @return  A 8-bit random number.
 */
static inline uint8_t cx_rng_u8(void)
{
    uint8_t r;
    cx_rng_no_throw((uint8_t *) &r, sizeof(uint8_t));
    return r;
}

typedef uint32_t (*cx_rng_u32_range_randfunc_t)(void);

/**
 * @brief   Generates a random 32-bit unsigned integer
 *          with a specified function.
 *
 * @details The generated number is taken in the range [a;b[
 *          with uniform distribution.
 *
 * @param[in] a        Inclusive low bound.
 *
 * @param[in] b        Exclusive high bound.
 *
 * @param[in] randfunc Function called to generate the random value.
 *
 * @return             A 32-bit random number.
 */
uint32_t cx_rng_u32_range_func(uint32_t a, uint32_t b, cx_rng_u32_range_randfunc_t randfunc);

/**
 * @brief   Generates a random 32-bit unsigned integer.
 *
 * @details The generated number is taken in the range [a;b[
 *          with uniform distribution.
 *
 * @param[in] a   Inclusive low bound.
 *
 * @param[in] b   Exclusive high bound.
 *
 * @return        A 32-bit random number.
 */
static inline uint32_t cx_rng_u32_range(uint32_t a, uint32_t b)
{
    return cx_rng_u32_range_func(a, b, cx_rng_u32);
}

/**
 * @brief   Generates a random buffer according to
 *          <a href="https://tools.ietf.org/html/rfc6979"> RFC6979 </a>.
 *
 * @param[in]  hash_id  Message digest algorithm identifier.
 *
 * @param[in]  x        ECDSA private key.
 *
 * @param[in]  x_len    Length of the key.
 *
 * @param[in]  h1       Hash of the message.
 *
 * @param[in]  h1_len   Length of the hash.
 *
 * @param[in]  q        Prime number that is a divisor of the curve order.
 *
 * @param[in]  q_len    Length of the prime number *q*.
 *
 * @param[out] out      Buffer for the output.
 *
 * @param[in]  out_len  Length of the output.
 *
 * @return              Error code:
 *                      - CX_OK on success
 *                      - CX_INVALID_PARAMETER
 */
WARN_UNUSED_RESULT cx_err_t cx_rng_rfc6979(cx_md_t        hash_id,
                                           const uint8_t *x,
                                           size_t         x_len,
                                           const uint8_t *h1,
                                           size_t         h1_len,
                                           const uint8_t *q,
                                           size_t         q_len,
                                           uint8_t       *out,
                                           size_t         out_len);
#endif  // HAVE_RNG

#ifdef HAVE_RNG_RFC6979
/**
 * @brief   Initialize a random buffer context according to
 *          <a href="https://tools.ietf.org/html/rfc6979"> RFC6979 </a>.
 *
 * @param[out]  rfc_ctx  Context to initialize
 *
 * @param[in]  hash_id  Message digest algorithm identifier.
 *
 * @param[in]  x        ECDSA private key.
 *
 * @param[in]  x_len    Length of the key.
 *
 * @param[in]  h1       Hash of the message.
 *
 * @param[in]  h1_len   Length of the hash.
 *
 * @param[in]  q        Prime number that is a divisor of the curve order.
 *
 * @param[in]  q_len    Length of the prime number *q*.
 *
 * @return              Error code:
 *                      - CX_OK on success
 *                      - CX_INVALID_PARAMETER
 */
WARN_UNUSED_RESULT cx_err_t cx_rng_rfc6979_init(cx_rnd_rfc6979_ctx_t *rfc_ctx,
                                                cx_md_t               hash_id,
                                                const uint8_t        *x,
                                                size_t                x_len,
                                                const uint8_t        *h1,
                                                size_t                h1_len,
                                                const uint8_t        *q,
                                                size_t                q_len);

/**
 * @brief   Generate a random buffer context according to
 *          <a href="https://tools.ietf.org/html/rfc6979"> RFC6979 </a>, from a context
 *          initialized with #cx_rng_rfc6979_init()
 *
 * @param[out]  rfc_ctx  Context initialized with #cx_rng_rfc6979_init
 *
 * @param[out] out      Buffer for the output.
 *
 * @param[in]  out_len  Length of the output.
 *
 *
 * @return              Error code:
 *                      - CX_OK on success
 *                      - CX_INVALID_PARAMETER
 */
WARN_UNUSED_RESULT cx_err_t cx_rng_rfc6979_next(cx_rnd_rfc6979_ctx_t *rfc_ctx,
                                                uint8_t              *out,
                                                size_t                out_len);
#endif  // HAVE_RNG_RFC6979

#endif  // LCX_RNG_H
