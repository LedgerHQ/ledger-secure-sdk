
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

#include "app_config.h"
#include "exceptions.h"
#include "lcx_rng.h"
#include "os_helpers.h"
#include "os_io.h"
#include "os_utils.h"
#include "os.h"
#include "os_io_seph_cmd.h"
#include <string.h>

#ifndef BOLOS_OS_UPGRADER_APP
void os_boot(void)
{
    // // TODO patch entry point when romming (f)
    // // set the default try context to nothing
#ifndef HAVE_BOLOS
    try_context_set(NULL);
#endif  // HAVE_BOLOS
}
#endif  // BOLOS_OS_UPGRADER_APP

void os_memset4(void *dst, unsigned int initval, unsigned int nbintval)
{
    while (nbintval--) {
        ((unsigned int *) dst)[nbintval] = initval;
    }
}

void os_xor(void *dst, void *src1, void *src2, unsigned int length)
{
#define SRC1 ((unsigned char const *) src1)
#define SRC2 ((unsigned char const *) src2)
#define DST  ((unsigned char *) dst)
    unsigned int l = length;
    // don't || to ensure all condition are evaluated
    while (!(!length && !l)) {
        length--;
        DST[length] = SRC1[length] ^ SRC2[length];
        l--;
    }
    // WHAT ??? glitch detected ?
    if (*((volatile unsigned int *) &l) != *((volatile unsigned int *) &length)) {
        THROW(EXCEPTION);
    }
}

char os_secure_memcmp(const void *src1, const void *src2, size_t length)
{
#define SRC1 ((unsigned char const *) src1)
#define SRC2 ((unsigned char const *) src2)
    unsigned int  l      = length;
    unsigned char xoracc = 0;
    // don't || to ensure all condition are evaluated
    while (!(!length && !l)) {
        length--;
        xoracc |= SRC1[length] ^ SRC2[length];
        l--;
    }
    // WHAT ??? glitch detected ?
    if (*(volatile unsigned int *) &l != *(volatile unsigned int *) &length) {
        THROW(EXCEPTION);
    }
    return xoracc;
}

#ifndef HAVE_BOLOS
#include "ledger_assert.h"
int main(void);

#define MAIN_LINKER_SCRIPT_LOCATION 0xC0DE0000
int compute_address_location(int address)
{
    // Compute location before relocation (sort of anti PIC)
    return address - (unsigned int) main + MAIN_LINKER_SCRIPT_LOCATION;
}

void os_longjmp(unsigned int exception)
{
#ifdef HAVE_DEBUG_THROWS
    // Send to the app the info of exception and LR for debug purpose
    DEBUG_THROW(exception);
#elif defined(HAVE_PRINTF)
    int lr_val;
    __asm volatile("mov %0, lr" : "=r"(lr_val));
    lr_val = compute_address_location(lr_val);

    PRINTF("exception[0x%04X]: LR=0x%08X\n", exception, lr_val);
#endif

    longjmp(try_context_get()->jmp_buf, exception);
}
#endif  // HAVE_BOLOS

#ifndef BOLOS_OS_UPGRADER_APP
void safe_desynch(void)
{
    volatile int a, b;
    unsigned int i;

    i = ((cx_rng_u32() & 0xFF) + 1u);
    a = b = 1;
    while (i--) {
        a = 1 + (b << (a / 2));
        b = cx_rng_u32();
    }
}
#endif  // BOLOS_OS_UPGRADER_APP

void u4be_encode(unsigned char *buffer, unsigned int offset, unsigned int value)
{
    U4BE_ENCODE(buffer, offset, value);
}

void u4le_encode(unsigned char *buffer, unsigned int offset, unsigned int value)
{
    U4LE_ENCODE(buffer, offset, value);
}

void os_reset(void)
{
    os_io_seph_cmd_se_reset();
    for (;;) {
    }
}

void *os_memmove(void *dest, const void *src, size_t n)
{
    return memmove(dest, src, n);
}

void *os_memcpy(void *dest, const void *src, size_t n)
{
    return memmove(dest, src, n);
}

int os_memcmp(const void *s1, const void *s2, size_t n)
{
    return memcmp(s1, s2, n);
}

void *os_memset(void *s, int c, size_t n)
{
    return memset(s, c, n);
}

int bytes_to_hex(char *out, size_t outl, const void *value, size_t len)
{
    const uint8_t *bytes = (const uint8_t *) value;
    const char    *hex   = "0123456789ABCDEF";

    if (outl < 2 * len + 1) {
        *out = '\0';
        return -1;
    }

    for (size_t i = 0; i < len; i++) {
        *out++ = hex[(bytes[i] >> 4) & 0xf];
        *out++ = hex[bytes[i] & 0xf];
    }
    *out = 0;
    return 0;
}

int bytes_to_lowercase_hex(char *out, size_t outl, const void *value, size_t len)
{
    const uint8_t *bytes = (const uint8_t *) value;
    const char    *hex   = "0123456789abcdef";

    if (outl < 2 * len + 1) {
        *out = '\0';
        return -1;
    }

    for (size_t i = 0; i < len; i++) {
        *out++ = hex[(bytes[i] >> 4) & 0xf];
        *out++ = hex[bytes[i] & 0xf];
    }
    *out = 0;
    return 0;
}

// BSS start and end defines addresses
extern void *_bss;
extern void *_ebss;

// This call will reset the value of the entire BSS segment
void os_explicit_zero_BSS_segment(void)
{
    explicit_bzero(&_bss, ((uintptr_t) &_ebss) - ((uintptr_t) &_bss));
}
