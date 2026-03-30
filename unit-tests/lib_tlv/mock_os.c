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
#include <ctype.h>

#include "os_pic.h"
#include "os_pki.h"
#include "cx.h"

bool is_printable_string(const char *str, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        if (!isprint((unsigned char) str[i])) {
            return false;
        }
    }
    return true;
}

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

size_t strlcpy(char *dst, const char *src, size_t size)
{
    size_t src_len = strlen(src);
    if (size > 0) {
        size_t copy_len = (src_len >= size) ? size - 1 : src_len;
        memcpy(dst, src, copy_len);
        dst[copy_len] = '\0';
    }
    return src_len;
}

bolos_err_t os_pki_get_info(uint8_t                  *key_usage,
                            uint8_t                  *trusted_name,
                            size_t                   *trusted_name_len,
                            cx_ecfp_384_public_key_t *public_key)
{
    (void) key_usage;
    (void) public_key;
    const char *name = "TestPartner";
    size_t      len  = strlen(name);
    if (trusted_name != NULL && trusted_name_len != NULL) {
        memcpy(trusted_name, name, len);
        *trusted_name_len = len;
    }
    return 0;
}
