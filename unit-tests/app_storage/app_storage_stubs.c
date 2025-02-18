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

#include <cmocka.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>

#include "app_storage_internal.h"
#include "os_pic.h"

const char           NVRAM_FILE_NAME[] = "nvram.bin";
extern app_storage_t app_storage_real;
#define as_blob ((uint8_t *) PIC(&app_storage_real))

void *pic(void *addr)
{
    return addr;
}

void zero_out_storage(void)
{
    memset(as_blob, 0, sizeof(app_storage_real));
}

void nvm_write(void *dst_addr, void *src_addr, size_t src_len)
{
    if (((const uint8_t *) dst_addr < as_blob)
        || ((const uint8_t *) dst_addr + src_len > &as_blob[sizeof(app_storage_real)])) {
        fprintf(stderr, "App NVRAM write attempt out of boundaries\n");
        fprintf(stderr, "dst_addr = %p\n", dst_addr);
        fprintf(stderr, "src_len = %zu\n", src_len);
        fprintf(stderr,
                "&as_blob[sizeof(app_storage_real)] = %p\n",
                &as_blob[sizeof(app_storage_real)]);
        return;
    }

    /* "Writing" - just using the global RAM array as the destination */
    size_t index = (const uint8_t *) dst_addr - as_blob;
    memcpy(&as_blob[index], src_addr, src_len);
}
