/*****************************************************************************
 *   (c) 2025 Ledger SAS.
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
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <string.h>

#ifdef UNIT_TESTING
// When defined cmocka redefine malloc/free which does not work well with
// address-sanitizer
#undef UNIT_TESTING
#include <cmocka.h>
#define UNIT_TESTING
#else
#include <cmocka.h>
#endif

#include <cmocka.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>

#include "os_pic.h"
#include "cx.h"

void *pic(void *addr)
{
    return addr;
}

void assert_exit(bool confirm)
{
    UNUSED(confirm);
    exit(0);
}

cx_err_t cx_sha256_init_no_throw(cx_sha256_t *hash)
{
    memset(hash, 0, sizeof(cx_sha256_t));
    return CX_OK;
}

cx_err_t cx_sha3_init_no_throw(cx_sha3_t *hash PLENGTH(sizeof(cx_sha3_t)), size_t size)
{
    UNUSED(hash);
    UNUSED(size);
    return CX_OK;
}

cx_err_t cx_keccak_init_no_throw(cx_sha3_t *hash PLENGTH(sizeof(cx_sha3_t)), size_t size)
{
    UNUSED(hash);
    UNUSED(size);
    return CX_OK;
}

cx_err_t cx_ripemd160_init_no_throw(cx_ripemd160_t *hash)
{
    UNUSED(hash);
    return CX_OK;
}

cx_err_t cx_sha512_init_no_throw(cx_sha512_t *hash)
{
    UNUSED(hash);
    return CX_OK;
}

cx_err_t cx_hash_update(cx_hash_t *ctx, const uint8_t *data, size_t len)
{
    UNUSED(ctx);
    UNUSED(data);
    UNUSED(len);
    return CX_OK;
}

cx_err_t cx_hash_final(cx_hash_t *ctx, uint8_t *digest)
{
    UNUSED(ctx);
    UNUSED(digest);
    return CX_OK;
}
