
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

#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include "app_config.h"
#include "decorators.h"
#include "os_math.h"

#if defined(HAVE_PRINTF) || defined(HAVE_SPRINTF)

// Buffer size for printed numbers
// Must be sufficient to contain a whole 64-bit number in binary plus sign and padding
#define PCBUF_SIZE 32

// Output function type
typedef void (*output_func_t)(const char *data, size_t len, void *context);

// Context for sprintf
typedef struct {
    char  *str;
    size_t str_size;
} sprintf_ctx_t;

// clang-format off
static const char g_pcHex[] = {
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'a', 'b', 'c', 'd', 'e', 'f',
};
static const char g_pcHex_cap[] = {
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'A', 'B', 'C', 'D', 'E', 'F',
};
// clang-format on

#ifdef HAVE_PRINTF
#include "os_io_seph_cmd.h"

static void printf_output(const char *data, size_t len, void *context)
{
    (void) context;
    os_io_seph_cmd_printf(data, len);
}
#endif

#ifdef HAVE_SPRINTF
static void sprintf_output(const char *data, size_t len, void *context)
{
    sprintf_ctx_t *ctx = (sprintf_ctx_t *) context;

    if (ctx->str_size == 0) {
        return;
    }

    len = MIN(len, ctx->str_size);
    memmove(ctx->str, data, len);
    ctx->str += len;
    ctx->str_size -= len;
}
#endif

// Helper function to apply padding and sign
static void apply_padding_output(char          *pcBuf,
                                 unsigned long *ulPos,
                                 unsigned long  ulWidth,
                                 unsigned long *ulNeg,
                                 char           cFill,
                                 unsigned long  ulNumLen,
                                 output_func_t  output,
                                 void          *output_ctx)
{
    unsigned long ulPaddingNeeded = 0;
    unsigned long ulActualLen     = ulNumLen;

    // If the number is negative, it will take one more character for the sign
    if (*ulNeg) {
        ulActualLen++;
    }

    // Calculate the necessary padding: Requested Width - Actual Width
    // If the requested width is 0 (e.g., %llu), the result is 0.
    if (ulWidth > ulActualLen) {
        ulPaddingNeeded = ulWidth - ulActualLen;
    }
    // If we're using '0' padding and the number is negative,
    // place the minus sign first, then add zeros
    if (*ulNeg && (cFill == '0')) {
        pcBuf[(*ulPos)++] = '-';
        *ulNeg            = 0;
    }

    // Output padding in chunks if it's larger than buffer
    while (ulPaddingNeeded > 0) {
        unsigned long chunkSize   = ulPaddingNeeded;
        unsigned long bufferSpace = PCBUF_SIZE - *ulPos;

        // If padding doesn't fit in current buffer, fill buffer and flush
        if (chunkSize > bufferSpace) {
            chunkSize = bufferSpace;
        }

        // Fill buffer with padding
        for (unsigned long i = 0; i < chunkSize; i++) {
            pcBuf[(*ulPos)++] = cFill;
        }
        ulPaddingNeeded -= chunkSize;

        // If buffer is full and we still have padding to add, flush it
        if (*ulPos >= PCBUF_SIZE && ulPaddingNeeded > 0) {
            output(pcBuf, *ulPos, output_ctx);
            *ulPos = 0;  // Reset buffer position
        }
    }

    // If negative and we haven't added the sign yet (space padding case),
    // add it now before the digits
    if (*ulNeg) {
        // Make sure there's space for the sign
        if (*ulPos >= PCBUF_SIZE) {
            output(pcBuf, *ulPos, output_ctx);
            *ulPos = 0;
        }
        pcBuf[(*ulPos)++] = '-';
    }
}

// Common formatting function
static int vformat_internal(output_func_t output,
                            void         *output_ctx,
                            const char   *format,
                            va_list       vaArgP)
{
    unsigned long ulIdx, ulValue, ulPos, ulCount, ulBase, ulNeg, ulStrlen, ulCap;
    char         *pcStr, pcBuf[PCBUF_SIZE], cFill;
    char          cStrlenSet;
    unsigned long ulLen;

    //
    // Loop while there are more characters in the string.
    //
    while (*format) {
        //
        // Find the first non-% character, or the end of the string.
        //
        for (ulIdx = 0; (format[ulIdx] != '%') && (format[ulIdx] != '\0'); ulIdx++) {
        }

        //
        // Write this portion of the string.
        //
        if (ulIdx > 0) {
            output(format, ulIdx, output_ctx);
        }

        //
        // Skip the portion of the string that was written.
        //
        format += ulIdx;

        //
        // See if the next character is a %.
        //
        if (*format == '%') {
            //
            // Skip the %.
            //
            format++;

            //
            // Set the digit count to zero, and the fill character to space
            // (i.e. to the defaults).
            //
            ulCount    = 0;
            cFill      = ' ';
            ulStrlen   = 0;
            cStrlenSet = 0;
            ulCap      = 0;
            ulBase     = 10;
            ulNeg      = 0;

        again:
            //
            // Determine how to handle the next character.
            //
            switch (*format++) {
                //
                // Handle the digit characters.
                //
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9': {
                    // If this is a zero, and it is the first digit, then the
                    // fill character is a zero instead of a space.
                    if ((format[-1] == '0') && (ulCount == 0)) {
                        cFill = '0';
                    }
                    ulCount *= 10;
                    ulCount += format[-1] - '0';
                    goto again;
                }

                case 'c': {
                    ulValue = va_arg(vaArgP, unsigned long);
                    output((char *) &ulValue, 1, output_ctx);
                    break;
                }

                case 'd': {
                    ulValue = va_arg(vaArgP, unsigned int);
                    ulPos   = 0;

                    if ((long) ulValue < 0) {
                        // Explicit cast to handle sign extension correctly
                        ulValue = -(long) ((int) ulValue);
                        ulNeg   = 1;
                    }
                    else {
                        ulNeg = 0;
                    }

                    ulBase = 10;
                    goto convert;
                }

#ifdef HAVE_SNPRINTF_FORMAT_LL
                case 'l': {
                    // Check if this is a 64-bit format: %ll followed by u, d, x, or X
                    if (*format == 'l'
                        && (*(format + 1) == 'u' || *(format + 1) == 'd' || *(format + 1) == 'x'
                            || *(format + 1) == 'X')) {
                        format++;  // skip second 'l' to point to the format specifier

                        // Get the 64-bit value from varargs
                        // Note: we always read as int64_t, then cast to uint64_t if needed
                        int64_t  slValue64 = va_arg(vaArgP, int64_t);
                        uint64_t ulValue64 = 0;

                        // Reset buffer position
                        ulPos = 0;
                        ulLen = 1;

                        // Determine the conversion base and handle signed values
                        if (*format == 'd') {
                            // %lld: signed decimal
                            ulBase = 10;
                            format++;
                            if (slValue64 < 0) {
                                ulNeg = 1;  // Mark as negative
                                ulValue64
                                    = (uint64_t) (-slValue64);  // Convert to positive for display
                            }
                            else {
                                ulNeg     = 0;
                                ulValue64 = (uint64_t) slValue64;
                            }
                        }
                        else if (*format == 'u') {
                            // %llu: unsigned decimal
                            ulBase = 10;
                            format++;
                            ulNeg     = 0;                     // Unsigned, no sign
                            ulValue64 = (uint64_t) slValue64;  // Reinterpret as unsigned
                        }
                        else if (*format == 'x') {
                            // %llx: hexadecimal lowercase
                            ulBase = 16;
                            ulCap  = 0;  // Use lowercase hex digits
                            format++;
                            ulNeg     = 0;
                            ulValue64 = (uint64_t) slValue64;
                        }
                        else if (*format == 'X') {
                            // %llX: hexadecimal uppercase
                            ulBase = 16;
                            ulCap  = 1;  // Use uppercase hex digits
                            format++;
                            ulNeg     = 0;
                            ulValue64 = (uint64_t) slValue64;
                        }

                        // Calculate the highest power of base needed for conversion
                        // This loop finds ulIdx64 such that ulIdx64 * base > value
                        // The second condition prevents overflow:
                        // if (ulIdx64 * base) / base != ulIdx64,
                        // then we've overflowed and should stop
                        uint64_t ulIdx64;
                        for (ulIdx64 = 1; (((ulIdx64 * ulBase) <= ulValue64)
                                           && (((ulIdx64 * ulBase) / ulBase) == ulIdx64));
                             ulIdx64 *= ulBase) {
                            ulLen++;
                        }

                        // Apply padding using helper function
                        apply_padding_output(
                            pcBuf, &ulPos, ulCount, &ulNeg, cFill, ulLen, output, output_ctx);

                        // Convert the number to ASCII digits
                        // We work from most significant to least significant digit
                        // ulIdx64 starts at the highest power of base (e.g., 1000000 for base 10)
                        // and divides down to 1
                        for (; ulIdx64; ulIdx64 /= ulBase) {
                            // Check if buffer is full, flush and reset
                            if (ulPos >= PCBUF_SIZE) {
                                output(pcBuf, ulPos, output_ctx);
                                ulPos = 0;
                            }

                            // Extract the digit at this position: (value / power) % base
                            // Example: for 12345, power=10000: (12345/10000)%10 = 1
                            if (!ulCap) {
                                pcBuf[ulPos++] = g_pcHex[(ulValue64 / ulIdx64) % ulBase];
                            }
                            else {
                                pcBuf[ulPos++] = g_pcHex_cap[(ulValue64 / ulIdx64) % ulBase];
                            }
                        }

                        // Output the formatted string
                        if (ulPos > 0) {
                            output(pcBuf, ulPos, output_ctx);
                        }
                        break;
                    }
                    goto error;
                }
#endif  // HAVE_SNPRINTF_FORMAT_LL

                case '.': {
                    if (format[0] == '*'
                        && (format[1] == 's' || format[1] == 'H' || format[1] == 'h')) {
                        format++;
                        ulStrlen   = va_arg(vaArgP, unsigned long);
                        cStrlenSet = 1;
                        goto again;
                    }
                    goto error;
                }

                case '*': {
                    if (*format == 's') {
                        ulStrlen   = va_arg(vaArgP, unsigned long);
                        cStrlenSet = 2;
                        goto again;
                    }
                    goto error;
                }

                case '-': {
                    cStrlenSet = 0;
                    goto again;
                }

                case 'H':
                    ulCap  = 1;
                    ulBase = 16;
                    goto case_s;
                case 'h':
                    ulCap  = 0;
                    ulBase = 16;
                    goto case_s;
                case 's':
                case_s : {
                    pcStr = va_arg(vaArgP, char *);

                    switch (cStrlenSet) {
                        case 0:
                            for (ulIdx = 0; pcStr[ulIdx] != '\0'; ulIdx++) {
                            }
                            break;
                        case 1:
                            ulIdx = ulStrlen;
                            break;
                        case 2:
                            if (pcStr[0] == '\0') {
                                for (ulCount = 0; ulCount < ulStrlen; ulCount++) {
                                    output(" ", 1, output_ctx);
                                }
                                goto s_pad;
                            }
                            goto error;
                        case 3:
                            goto again;
                    }

                    switch (ulBase) {
                        default:
                            output(pcStr, ulIdx, output_ctx);
                            break;
                        case 16: {
                            unsigned char nibble1, nibble2;
                            unsigned int  idx = 0;
                            for (ulCount = 0; ulCount < ulIdx; ulCount++) {
                                nibble1 = (pcStr[ulCount] >> 4) & 0xF;
                                nibble2 = pcStr[ulCount] & 0xF;
                                if (ulCap) {
                                    pcBuf[idx++] = g_pcHex_cap[nibble1];
                                    pcBuf[idx++] = g_pcHex_cap[nibble2];
                                }
                                else {
                                    pcBuf[idx++] = g_pcHex[nibble1];
                                    pcBuf[idx++] = g_pcHex[nibble2];
                                }
                                if (idx + 1 >= sizeof(pcBuf)) {
                                    output(pcBuf, idx, output_ctx);
                                    idx = 0;
                                }
                            }
                            if (idx != 0) {
                                output(pcBuf, idx, output_ctx);
                            }
                            break;
                        }
                    }

                s_pad:
                    if (ulCount > ulIdx) {
                        ulCount -= ulIdx;
                        while (ulCount--) {
                            output(" ", 1, output_ctx);
                        }
                    }
                    break;
                }

#ifdef HAVE_SNPRINTF_FORMAT_U
                case 'u': {
                    ulValue = va_arg(vaArgP, unsigned int);
                    ulPos   = 0;
                    ulBase  = 10;
                    ulNeg   = 0;
                    goto convert;
                }
#endif  // HAVE_SNPRINTF_FORMAT_U

                case 'X':
                    ulCap = 1;
                    FALL_THROUGH;
                case 'x':
                case 'p': {
                    // Note : 'p' is generally a long/pointer, so we can separate 'p' if we want to
                    // be strict, but 'unsigned long' for 'p' is correct on PC (64bit) and ARM
                    // (32bit). For 'x' and 'X', it is necessary to use int.
                    if (*(format - 1) == 'p') {
                        ulValue = va_arg(vaArgP, unsigned long);  // %p is a pointer (native size)
                    }
                    else {
                        ulValue = va_arg(vaArgP, unsigned int);  // %x is a 32-bit integer
                    }
                    ulPos  = 0;
                    ulBase = 16;
                    ulNeg  = 0;

                convert:
                    ulLen = 1;
                    for (ulIdx = 1;
                         (((ulIdx * ulBase) <= ulValue) && (((ulIdx * ulBase) / ulBase) == ulIdx));
                         ulIdx *= ulBase) {
                        ulLen++;
                    }

                    // Apply padding using helper function
                    apply_padding_output(
                        pcBuf, &ulPos, ulCount, &ulNeg, cFill, ulLen, output, output_ctx);

                    // Convert 32-bit value to string
                    for (; ulIdx; ulIdx /= ulBase) {
                        // Check if buffer is full, flush and reset
                        if (ulPos >= PCBUF_SIZE) {
                            output(pcBuf, ulPos, output_ctx);
                            ulPos = 0;
                        }

                        if (!ulCap) {
                            pcBuf[ulPos++] = g_pcHex[(ulValue / ulIdx) % ulBase];
                        }
                        else {
                            pcBuf[ulPos++] = g_pcHex_cap[(ulValue / ulIdx) % ulBase];
                        }
                    }

                    if (ulPos > 0) {
                        output(pcBuf, ulPos, output_ctx);
                    }
                    break;
                }

                case '%': {
                    output("%", 1, output_ctx);
                    break;
                }

                error:
                default: {
#ifdef HAVE_SNPRINTF_DEBUG
                    output("ERROR", 5, output_ctx);
#endif
                    break;
                }
            }
        }
    }

    return 0;
}

#ifdef HAVE_PRINTF
void screen_printf(const char *format, ...) __attribute__((weak, alias("mcu_usb_printf")));

void mcu_usb_printf(const char *format, ...)
{
    va_list vaArgP;

    if (format == 0) {
        return;
    }

    va_start(vaArgP, format);
    vformat_internal(printf_output, NULL, format, vaArgP);
    va_end(vaArgP);
}

#endif  // HAVE_PRINTF

#ifdef HAVE_SPRINTF
int snprintf(char *str, size_t str_size, const char *format, ...)
{
    va_list       vaArgP;
    sprintf_ctx_t ctx;

    if (str == NULL || str_size < 1) {
        return 0;
    }

    memset(str, 0, str_size);
    str_size--;

    if (str_size < 1) {
        return 0;
    }

    ctx.str      = str;
    ctx.str_size = str_size;

    va_start(vaArgP, format);
    vformat_internal(sprintf_output, &ctx, format, vaArgP);
    va_end(vaArgP);

    return 0;
}
#endif  // HAVE_SPRINTF

#endif  // defined(HAVE_PRINTF) || defined(HAVE_SPRINTF)
