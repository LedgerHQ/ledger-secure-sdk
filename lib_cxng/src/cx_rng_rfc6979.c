
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

#ifdef HAVE_RNG_RFC6979

#include <string.h>
#include "cx_rng_rfc6979.h"
#include "cx_hmac.h"
#include "cx_hash.h"
#include "cx_ram.h"
#include "exceptions.h"
#include "errors.h"

/* ----------------------------------------------------------------------- */
/* Compute r = a-b                                                         */
/*                                                                         */
/*  Params:                                                                */
/*   r  : result                                                           */
/*   a  : first operand                                                    */
/*   b  : first operand                                                    */
/*   len: bytes length of a,b and r                                        */
/* ----------------------------------------------------------------------- */
static void cx_rfc6979_sub(uint8_t *r, uint8_t *a, uint8_t *b, size_t len)
{
    uint32_t c;
    c = 0;
    while (len--) {
        c      = a[len] - b[len] - c;
        r[len] = c & 0xFF;
        c      = c & 0xFFFFFF00 ? 1 : 0;
    }
}

/* ----------------------------------------------------------------------- */
/* Convert an arbitrary bits string to BE int of r_len bits                */
/*  -> keep only the first q_len bits of b,                                */
/*  -> expand to r_len bits                                                */
/*                                                                         */
/*  Params:                                                                */
/*    b    : b string to convert                                           */
/*    b_len: bits length os b, shall be multiple of 8, true by design      */
/* ----------------------------------------------------------------------- */
static size_t cx_rfc6979_bits2int(cx_rnd_rfc6979_ctx_t *rfc_ctx,
                                  const uint8_t        *b,
                                  size_t                b_len,
                                  uint8_t              *b_out)
{
    if (b_len > rfc_ctx->q_len) {
        uint8_t right_shift = (b_len - rfc_ctx->q_len) & 7;
        // For example copy a SHA384 digest in a 256-bit buffer: 384 > 256
        // bits2int copy the high 256 bits from b into b_out
        if (right_shift == 0) {
            // Easy case: copy bytes directly
            memmove(b_out, b, rfc_ctx->r_len >> 3);
        }
        else {
            // Shift bits of b from (b_len - rfc_ctx->q_len) bytes to the right, into b_out
            uint8_t carry = 0;
            size_t  i, rlen = rfc_ctx->r_len >> 3;

            for (i = 0; i < rlen; i++) {
                uint8_t x = b[i];
                b_out[i]  = carry | (x >> right_shift);
                carry     = (x << (8 - right_shift)) & 0xff;
            }
        }
    }
    else {
        // Pad b with zeros
        b_len              = b_len >> 3;
        size_t padding_len = (rfc_ctx->r_len >> 3) - b_len;
        memset(b_out, 0, padding_len);
        memmove(b_out + padding_len, b, b_len);
    }
    return rfc_ctx->r_len;
}

/* ----------------------------------------------------------------------- */
/* Convert an arbitrary bits string to BE int of r_len bits, lesser that q */
/*  -> z = bits2int(bs)                                                    */
/*  -> z = z-q if z>q                                                      */
/*                                                                         */
/*  Params:                                                                */
/*    b    : b string to convert                                           */
/*    b_len: bits length os b, shall be multiple of 8, true by design      */
/* ----------------------------------------------------------------------- */
static void cx_rfc6979_bits2octets(cx_rnd_rfc6979_ctx_t *rfc_ctx,
                                   const uint8_t        *b,
                                   size_t                b_len,
                                   uint8_t              *b_out)
{
    cx_rfc6979_bits2int(rfc_ctx, b, b_len, b_out);
    if (memcmp(b_out, rfc_ctx->q, rfc_ctx->r_len >> 3) > 0) {
        cx_rfc6979_sub(b_out, b_out, rfc_ctx->q, rfc_ctx->r_len >> 3);
    }
}

/* ----------------------------------------------------------------------- */
/* Convert an integer bits string to BE int of r_len bits                  */
/*                                                                         */
/*  Params:                                                                */
/*    i    : integer to convert                                            */
/*    i_len: bits length os i, shall be multiple of 8, true by design      */
/* ----------------------------------------------------------------------- */
static void cx_rfc6979_int2octets(cx_rnd_rfc6979_ctx_t *rfc_ctx,
                                  const uint8_t        *i,
                                  size_t                i_len,
                                  uint8_t              *b_out)
{
    int32_t delta;
    delta = (i_len >> 3) - (rfc_ctx->r_len >> 3);
    if (delta < 0) {
        delta = -delta;
        memcpy(b_out + delta, i, i_len >> 3);
        while (delta--) {
            b_out[delta] = 0;
        }
    }
    else {
        // assume first bytes are null
        memcpy(b_out, i + delta, rfc_ctx->r_len >> 3);
    }
}

/* ----------------------------------------------------------------------- */
/* Compute effective bits length of 'b'                                    */
/*                                                                         */
/*  Params:                                                                */
/*    b    : b                                                             */
/*    b_len: bytes length b                                                */
/* ----------------------------------------------------------------------- */
static uint32_t cx_rfc6979_bitslength(const uint8_t *a, size_t a_len)
{
    uint8_t b;

    while (*a == 0) {
        a++;
        a_len--;
        if (a_len == 0) {
            return 0;
        }
    }
    // note: here len != 0 && *a != 0
    a_len = a_len * 8;
    b     = *a;
    while ((b & 0x80) == 0) {
        a_len--;
        b = b << 1;
    }
    return a_len;
}

static cx_err_t cx_rfc6979_hmacVK(cx_rnd_rfc6979_ctx_t *rfc_ctx,
                                  int8_t                opt,
                                  const uint8_t        *x,
                                  size_t                x_len,
                                  const uint8_t        *h1,
                                  size_t                h1_len,
                                  /*const uint8_t *additional_input, size_t additional_input_len,*/
                                  uint8_t *out)
{
    size_t   len;
    cx_err_t error;

    len = rfc_ctx->md_len;
    CX_CHECK(cx_hmac_init(&rfc_ctx->hmac, rfc_ctx->hash_id, rfc_ctx->k, rfc_ctx->md_len));
    if (opt >= 0) {
        rfc_ctx->v[rfc_ctx->md_len] = opt;
        len++;
    }
    CX_CHECK(cx_hmac_update(&rfc_ctx->hmac, rfc_ctx->v, len));
    if (x) {
        cx_rfc6979_int2octets(rfc_ctx, x, x_len * 8, rfc_ctx->tmp);
        CX_CHECK(cx_hmac_update(&rfc_ctx->hmac, rfc_ctx->tmp, rfc_ctx->r_len >> 3));
    }
    if (h1) {
        cx_rfc6979_bits2octets(rfc_ctx, h1, h1_len * 8, rfc_ctx->tmp);
        CX_CHECK(cx_hmac_update(&rfc_ctx->hmac, rfc_ctx->tmp, rfc_ctx->r_len >> 3));
    }
    /*
    if (additional_input) {
      CX_CHECK(cx_hmac_update(&rfc_ctx->hmac, additional_input, additional_input_len));
    }
    */
    len = rfc_ctx->md_len;
    CX_CHECK(cx_hmac_final(&rfc_ctx->hmac, out, &len));
end:
    return error;
}

cx_err_t cx_rng_rfc6979_init(cx_rnd_rfc6979_ctx_t *rfc_ctx,
                             cx_md_t               hash_id,
                             const uint8_t        *x,
                             size_t                x_len,
                             const uint8_t        *h1,
                             size_t                h1_len,
                             const uint8_t        *q,
                             size_t                q_len
                             /*const uint8_t *additional_input, size_t additional_input_len*/)
{
    cx_err_t              error;
    const cx_hash_info_t *hash_info = cx_hash_get_info(hash_id);
    if (hash_info == NULL || hash_info->output_size == 0) {
        return CX_INVALID_PARAMETER;
    }

    // setup params
    memcpy(rfc_ctx->q, q, q_len);
    rfc_ctx->q_len   = cx_rfc6979_bitslength(q, q_len);
    rfc_ctx->r_len   = (rfc_ctx->q_len + 7) & ~7;
    rfc_ctx->hash_id = hash_id;
    rfc_ctx->md_len  = hash_info->output_size;

    // STEP A: h1 = HASH(m)
    //  input is h1

    // Step B: V = 0x01...01  @digest_len
    memset(rfc_ctx->v, 0x01, rfc_ctx->md_len);

    // Step C: K = 0x00...00  @digest_len
    memset(rfc_ctx->k, 0x00, rfc_ctx->md_len);

    // Step D:  K = HMAC (K, V || 0x00 || int2octets(x) || bits2octetc(h1) [ || additional_input])
    CX_CHECK(cx_rfc6979_hmacVK(rfc_ctx,
                               0,
                               x,
                               x_len,
                               h1,
                               h1_len,
                               rfc_ctx->k /*,  additional_input, additional_input_len*/));

    // Step E: V = HMAC (K, V).
    CX_CHECK(cx_rfc6979_hmacVK(rfc_ctx, -1, NULL, 0, NULL, 0, rfc_ctx->v));

    // Step F:  K = HMAC (K, V || 0x01 || int2octets(x) || bits2octetc(h1) [ || additional_input])
    CX_CHECK(cx_rfc6979_hmacVK(rfc_ctx,
                               0x01,
                               x,
                               x_len,
                               h1,
                               h1_len,
                               rfc_ctx->k /*,  additional_input, additional_input_len*/));

    // Step G:  V = HMAC (K, V).
    CX_CHECK(cx_rfc6979_hmacVK(rfc_ctx, -1, NULL, 0, NULL, 0, rfc_ctx->v));
end:
    return error;
}

cx_err_t cx_rng_rfc6979_next(cx_rnd_rfc6979_ctx_t *rfc_ctx, uint8_t *out, size_t out_len)
{
    size_t   t_Blen;
    size_t   r_Blen;
    bool     found;
    cx_err_t error = CX_OK;

    if ((out_len * 8) < rfc_ctx->r_len) {
        return CX_INVALID_PARAMETER;
    }

    r_Blen = rfc_ctx->r_len >> 3;
    found  = false;
    while (!found) {
        // Step H1:
        t_Blen = 0;

        // Step H2:
        //  while t_Blen<qlen
        //    V = HMAC (K, V).
        //    T = T || V
        while (t_Blen < r_Blen) {
            CX_CHECK(cx_rfc6979_hmacVK(rfc_ctx, -1, NULL, 0, NULL, 0, rfc_ctx->v));
            if (rfc_ctx->md_len > (r_Blen - t_Blen)) {
                memcpy(out + t_Blen, rfc_ctx->v, r_Blen - t_Blen);
                t_Blen = r_Blen;
            }
            else {
                memcpy(out + t_Blen, rfc_ctx->v, rfc_ctx->md_len);
                t_Blen += rfc_ctx->md_len;
            }
        }

        // STEP H3: k = bits2int(T)
        cx_rfc6979_bits2int(rfc_ctx, out, t_Blen * 8, out);
        if (memcmp(out, rfc_ctx->q, rfc_ctx->r_len >> 3) < 0) {
            found = true;
        }

        // STEP H3 bis:
        //  K = HMAC (K, V || 0).
        CX_CHECK(cx_rfc6979_hmacVK(rfc_ctx, 0, NULL, 0, NULL, 0, rfc_ctx->k));
        //  V = HMAC (K, V).
        CX_CHECK(cx_rfc6979_hmacVK(rfc_ctx, -1, NULL, 0, NULL, 0, rfc_ctx->v));
    }
end:
    return error;
}

cx_err_t cx_rng_rfc6979(cx_md_t        hash_id,
                        const uint8_t *x,
                        size_t         x_len,
                        const uint8_t *h1,
                        size_t         h1_len,
                        const uint8_t *q,
                        size_t         q_len,
                        uint8_t       *out,
                        size_t         out_len)
{
    cx_err_t error;
    CX_CHECK(cx_rng_rfc6979_init(&G_cx.rfc6979, hash_id, x, x_len, h1, h1_len, q, q_len));
    CX_CHECK(cx_rng_rfc6979_next(&G_cx.rfc6979, out, out_len));
end:
    return error;
}
#endif  // HAVE_RNG_RFC6979
