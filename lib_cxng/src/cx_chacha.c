
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

#if defined(HAVE_CHACHA)

#include "lcx_chacha.h"
#include "cx_ram.h"
#include "cx_utils.h"
#include "os_math.h"
#include "os_utils.h"
#include <stddef.h>
#include <string.h>

#define CHACHA_STATE_ARRAY_SIZE 16
#define CHACHA_BLOCK_SIZE       64

#define ROTATE(v, n) (cx_rotl(v, n))
#define XOR(v, w)    ((v) ^ (w))
#define PLUS(v, w)   ((uint32_t) (v + w))

static const uint8_t constants[16] = "expand 32-byte k";

/* Chacha-20 quarter round function */
STATIC void cx_chacha_quarter_round(uint32_t *a, uint32_t *b, uint32_t *c, uint32_t *d)
{
    *a = PLUS(*a, *b);
    *d = ROTATE(XOR(*d, *a), 16);
    *c = PLUS(*c, *d);
    *b = ROTATE(XOR(*b, *c), 12);
    *a = PLUS(*a, *b);
    *d = ROTATE(XOR(*d, *a), 8);
    *c = PLUS(*c, *d);
    *b = ROTATE(XOR(*b, *c), 7);
}

static void cx_chacha_process_block(cx_chacha_context_t *ctx)
{
    unsigned int i;
    uint32_t    *x = (uint32_t *) ctx->block;
    for (i = 0; i < CHACHA_STATE_ARRAY_SIZE; i++) {
        x[i] = ctx->state[i];
    }
    for (i = 0; i < ctx->nrounds; i += 2) {
        /* Column rounds : apply the quarter-round function to each column */
        cx_chacha_quarter_round(&x[0], &x[4], &x[8], &x[12]);
        cx_chacha_quarter_round(&x[1], &x[5], &x[9], &x[13]);
        cx_chacha_quarter_round(&x[2], &x[6], &x[10], &x[14]);
        cx_chacha_quarter_round(&x[3], &x[7], &x[11], &x[15]);

        /* Diagonal rounds : apply the quarter-round function to the 'diagonals' */
        cx_chacha_quarter_round(&x[0], &x[5], &x[10], &x[15]);
        cx_chacha_quarter_round(&x[1], &x[6], &x[11], &x[12]);
        cx_chacha_quarter_round(&x[2], &x[7], &x[8], &x[13]);
        cx_chacha_quarter_round(&x[3], &x[4], &x[9], &x[14]);
    }
    for (i = 0; i < CHACHA_STATE_ARRAY_SIZE; i++) {
        x[i] = PLUS(x[i], ctx->state[i]);
    }
}

void cx_chacha_init(cx_chacha_context_t *ctx, uint32_t nrounds)
{
    memset(ctx, 0, sizeof(cx_chacha_context_t));
    ctx->nrounds = nrounds;
}

cx_err_t cx_chacha_set_key(cx_chacha_context_t *ctx, const uint8_t *key, size_t key_len)
{
    if (key_len != 32) {
        return CX_INVALID_PARAMETER_VALUE;
    }

    /* Initialize the state array */
    ctx->state[4]  = U4LE(key, 0);
    ctx->state[5]  = U4LE(key, 4);
    ctx->state[6]  = U4LE(key, 8);
    ctx->state[7]  = U4LE(key, 12);
    ctx->state[8]  = U4LE(key, 16);
    ctx->state[9]  = U4LE(key, 20);
    ctx->state[10] = U4LE(key, 24);
    ctx->state[11] = U4LE(key, 28);

    ctx->state[0] = U4LE(constants, 0);
    ctx->state[1] = U4LE(constants, 4);
    ctx->state[2] = U4LE(constants, 8);
    ctx->state[3] = U4LE(constants, 12);

    return CX_OK;
}

cx_err_t cx_chacha_start(cx_chacha_context_t *ctx, const uint8_t *iv, size_t iv_len)
{
    if (iv_len != 16) {
        return CX_INVALID_PARAMETER_VALUE;
    }
    /* Set the counter value */
    ctx->state[12] = U4BE(iv, 0);
    /* Set the nonce value */
    ctx->state[13] = U4LE(iv, 4);
    ctx->state[14] = U4LE(iv, 8);
    ctx->state[15] = U4LE(iv, 12);
    ctx->pos       = 0;

    return CX_OK;
}

cx_err_t cx_chacha_update(cx_chacha_context_t *ctx,
                          const uint8_t       *input,
                          uint8_t             *output,
                          size_t               len)
{
    unsigned int i, n;
    uint8_t     *key_stream;

    while (len > 0) {
        if ((0 == ctx->pos) || (ctx->pos >= 64)) {
            cx_chacha_process_block(ctx);
            ctx->state[12]++;
            if (0 == ctx->state[12]) {
                ctx->state[13]++;
            }
            ctx->pos = 0;
        }
        n          = MIN(len, CHACHA_BLOCK_SIZE - ctx->pos);
        key_stream = (uint8_t *) ctx->block + ctx->pos;
        if (NULL == input) {
            for (i = 0; i < n; i++) {
                output[i] = key_stream[i];
            }
        }
        else {
            for (i = 0; i < n; i++) {
                output[i] = input[i] ^ key_stream[i];
            }
        }
        input += n;
        output += n;
        ctx->pos += n;
        len -= n;
    }
    return CX_OK;
}

cx_err_t cx_chacha_cipher(uint32_t       nrounds,
                          const uint8_t *key,
                          size_t         key_len,
                          const uint8_t *iv,
                          size_t         iv_len,
                          const uint8_t *input,
                          uint8_t       *output,
                          size_t         len)
{
    cx_chacha_context_t *ctx = &G_cx.chacha;
    cx_err_t             error;
    cx_chacha_init(ctx, nrounds);
    CX_CHECK(cx_chacha_set_key(ctx, key, key_len));
    CX_CHECK(cx_chacha_start(ctx, iv, iv_len));
    CX_CHECK(cx_chacha_update(ctx, input, output, len));

end:
    explicit_bzero(ctx, sizeof(cx_chacha_context_t));
    return error;
}
#endif  // HAVE_CHACHA
