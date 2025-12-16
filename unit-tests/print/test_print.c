#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include "os_print.h"
#include "os_helpers.h"
#include "os_utils.h"

// Buffer to accumulate outputs
static char   output_buffer[1024];
static size_t output_pos = 0;

// Mock implementation of os_io_seph_cmd_printf to capture output
void os_io_seph_cmd_printf(const char *data, size_t len)
{
    // Accumulate the data in the buffer
    if (output_pos + len < sizeof(output_buffer)) {
        memcpy(output_buffer + output_pos, data, len);
        output_pos += len;
        output_buffer[output_pos] = '\0';
    }
}

// Setup function to reset the buffer before each test
static int setup(void **state)
{
    UNUSED(state);
    memset(output_buffer, 0, sizeof(output_buffer));
    output_pos = 0;
    return 0;
}

// ****************************************************************************
// Unit Tests for mcu_usb_printf (PRINTF)
// ****************************************************************************

// Test printf with %llu
static void test_printf_llu(void **state)
{
    UNUSED(state);
    uint64_t value = 9876543210123456789ULL;

    mcu_usb_printf("(DEC) unsigned_value: %llu", value);
    assert_string_equal(output_buffer, "(DEC) unsigned_value: 9876543210123456789");
}

// Test printf with %lld
static void test_printf_lld(void **state)
{
    UNUSED(state);
    int64_t value = -123456789012345LL;

    mcu_usb_printf("(DEC) signed_value: %lld", value);
    assert_string_equal(output_buffer, "(DEC) signed_value: -123456789012345");
}

// Test printf with %llx
static void test_printf_llx(void **state)
{
    UNUSED(state);
    uint64_t value = 0x123456789ABCDEF0ULL;

    mcu_usb_printf("(HEX) value: 0x%llx", value);
    assert_string_equal(output_buffer, "(HEX) value: 0x123456789abcdef0");
}

// Test printf with %llX with padding
static void test_printf_llX_padded(void **state)
{
    UNUSED(state);
    uint64_t value = 0x123456789ABCDEF0ULL;

    mcu_usb_printf("(HEX) value: 0x%020llX", value);
    assert_string_equal(output_buffer, "(HEX) value: 0x0000123456789ABCDEF0");
}

// Test printf with %d with padding
static void test_printf_d_padded(void **state)
{
    UNUSED(state);
    int32_t value = 4662;

    mcu_usb_printf("Padding: '%25d'", value);
    assert_string_equal(output_buffer, "Padding: '                     4662'");
}

// Test printf with long padding
static void test_printf_long_padding(void **state)
{
    UNUSED(state);
    int64_t value = 4661ULL;

    mcu_usb_printf("Padding: '%67lld'", value);
    assert_string_equal(
        output_buffer,
        "Padding: '                                                               4661'");
}

// Test printf with short padding
static void test_printf_short_padding(void **state)
{
    UNUSED(state);
    int64_t value = 123454661ULL;

    mcu_usb_printf("Padding: '%5lld'", value);
    assert_string_equal(output_buffer, "Padding: '123454661'");
}

// ****************************************************************************
// Unit Tests for snprintf
// ****************************************************************************

// Test snprintf with %lld
static void test_snprintf_lld(void **state)
{
    UNUSED(state);
    int64_t value      = -123456789012345LL;
    char    expected[] = "Dec: -123456789012345";
    int     written;

    // use cast because on x86 platform, type can be different (in default standard headers)
    written = snprintf(output_buffer, sizeof(output_buffer), "Dec: %lld", (long long) value);
    assert_string_equal(output_buffer, expected);
    assert_int_equal(written, strlen(expected));
}

// Test snprintf with %llX with padding
static void test_snprintf_llX_padded(void **state)
{
    UNUSED(state);
    uint64_t value      = 0x123456789ABCDEF0ULL;
    char     expected[] = "Hex: 0x123456789ABCDEF0";
    int      written;

    // use cast because on x86 platform, type can be different (in default standard headers)
    written = snprintf(
        output_buffer, sizeof(output_buffer), "Hex: 0x%016llX", (unsigned long long) value);
    assert_string_equal(output_buffer, expected);
    assert_int_equal(written, strlen(expected));
}

// Test snprintf with small buffer
static void test_snprintf_small_buffer(void **state)
{
    UNUSED(state);
    char buf[1];
    int  written;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
    written = snprintf(buf, 1, "Hello");
#pragma GCC diagnostic pop
    assert_string_equal(buf, "");  // Buffer should only contain '\0'
    // snprintf should return the number of bytes that would have been written
    // "Hello" = 5 bytes (excluding '\0')
    assert_int_equal(written, 5);
}

// ****************************************************************************
// Unit Tests for snprintf with UTF-8 strings
// ****************************************************************************

// Test snprintf with small buffer
static void test_snprintf_utf8_small_buffer(void **state)
{
    UNUSED(state);
    // Used to check snprintf didn't wrote any byte after the expected limit
    char canary_mark[] = "Canary";
    char buf[2 + 1 + sizeof(canary_mark)];  // sizeof take in account the '\0'
    int  written;

    // Write the Canary
    strcpy(&buf[2 + 1], canary_mark);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
    written = snprintf(buf, 2 + 1, "été");  // 0xC3 0xA9 0x74 0xC3 0xA9 0x00
#pragma GCC diagnostic pop
    assert_string_equal(buf, "é");                  // Buffer should only contain 'é'
    assert_string_equal(&buf[2 + 1], canary_mark);  // Check if Canary is still 'alive'
    // snprintf should return the number of bytes that would have been written
    // "été" = 5 bytes (excluding '\0')
    assert_int_equal(written, 5);

    // Same, but with only 1 byte => the UTF-8 character 'é' will be lost
    // Write the Canary
    strcpy(&buf[1 + 1], canary_mark);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
    written = snprintf(buf, 1 + 1, "été");  // 0xC3 0xA9 0x74 0xC3 0xA9 0x00
#pragma GCC diagnostic pop
    // Improved snprintf, who doesn't cut a utf-8 character
    assert_int_equal((uint8_t) buf[0], 0);  // The invalid 0xC3 character was replaced by a '\0'
    assert_int_equal(buf[1], 0x00);         // '\0'
    assert_string_equal(&buf[1 + 1], canary_mark);  // Check if Canary is still 'alive'
    // snprintf should return the number of bytes that would have been written
    // "été" = 5 bytes (excluding '\0')
    assert_int_equal(written, 5);
}

// Check an UTF-8 string ending with a multi-bytes character using 2 bytes
static void test_snprintf_utf8_strings_2(void **state)
{
    UNUSED(state);
    char canary_mark[]
        = "Canary";  // Used to check snprintf didn't wrote any byte after the expected limit
    char string[] = "toto_àé";  // toto_à + 0xC3 0xA9

    char buf[sizeof(string) + sizeof(canary_mark)];  // sizeof take in account the '\0'
    int  written;

    size_t len = strlen(string);  // strlen returns the number of bytes without the ending '\0'
    assert_int_equal(len, 5 + 2 + 2);
    assert_int_equal(len + 1, sizeof(string));

    // First pass: write the full string
    strcpy(&buf[len + 1], canary_mark);  // Write the Canary at the right location
    written = snprintf(buf, len + 1, "%s", string);
    assert_int_equal(written, len);

    assert_string_equal(buf, string);                 // Check the full string was copied
    assert_string_equal(&buf[len + 1], canary_mark);  // Check if Canary is still 'alive'

    // Second pass: write the full string minus 1 byte => the last 2 bytes will be removed
    strcpy(&buf[len + 1 - 1], canary_mark);  // Write the Canary at the right location
    written = snprintf(buf, len + 1 - 1, "%s", string);
    assert_int_equal(written, len);

    assert_string_equal(buf, "toto_à");                   // Check the right string was copied
    assert_string_equal(&buf[len + 1 - 1], canary_mark);  // Check if Canary is still 'alive'

    // Third pass: write the full string minus 2 bytes => the last 2 bytes will be removed
    strcpy(&buf[len + 1 - 2], canary_mark);  // Write the Canary at the right location
    written = snprintf(buf, len + 1 - 2, "%s", string);
    assert_int_equal(written, len);

    assert_string_equal(buf, "toto_à");                   // Check the right string was copied
    assert_string_equal(&buf[len + 1 - 2], canary_mark);  // Check if Canary is still 'alive'

    // Last pass: write the full string minus 3 bytes => the last 2+2 bytes will be removed
    strcpy(&buf[len + 1 - 3], canary_mark);  // Write the Canary at the right location
    written = snprintf(buf, len + 1 - 3, "%s", string);
    assert_int_equal(written, len);

    assert_string_equal(buf, "toto_");                    // Check the right string was copied
    assert_string_equal(&buf[len + 1 - 3], canary_mark);  // Check if Canary is still 'alive'
}

// Check an UTF-8 string ending with a multi-bytes character using 3 bytes
static void test_snprintf_utf8_strings_3(void **state)
{
    UNUSED(state);
    char canary_mark[]
        = "Canary";  // Used to check snprintf didn't wrote any byte after the expected limit
    char string[] = "toto_àด";  // toto_à + 0xE0 0xB8 0x94

    char buf[sizeof(string) + sizeof(canary_mark)];  // sizeof take in account the '\0'
    int  written;

    size_t len = strlen(string);  // strlen returns the number of bytes without the ending '\0'
    assert_int_equal(len, 5 + 2 + 3);
    assert_int_equal(len + 1, sizeof(string));

    // First pass: write the full string
    strcpy(&buf[len + 1], canary_mark);  // Write the Canary at the right location
    written = snprintf(buf, len + 1, "%s", string);
    assert_int_equal(written, len);

    assert_string_equal(buf, string);                 // Check the full string was copied
    assert_string_equal(&buf[len + 1], canary_mark);  // Check if Canary is still 'alive'

    // Second pass: write the full string minus 1 byte => the last 3 bytes will be removed
    strcpy(&buf[len + 1 - 1], canary_mark);  // Write the Canary at the right location
    written = snprintf(buf, len + 1 - 1, "%s", string);
    assert_int_equal(written, len);

    assert_string_equal(buf, "toto_à");                   // Check the right string was copied
    assert_string_equal(&buf[len + 1 - 1], canary_mark);  // Check if Canary is still 'alive'

    // Third pass: write the full string minus 2 bytes => the last 3 bytes will be removed
    strcpy(&buf[len + 1 - 2], canary_mark);  // Write the Canary at the right location
    written = snprintf(buf, len + 1 - 2, "%s", string);
    assert_int_equal(written, len);

    assert_string_equal(buf, "toto_à");                   // Check the right string was copied
    assert_string_equal(&buf[len + 1 - 2], canary_mark);  // Check if Canary is still 'alive'

    // fourth pass: write the full string minus 3 bytes => the last 3 bytes will be removed
    strcpy(&buf[len + 1 - 3], canary_mark);  // Write the Canary at the right location
    written = snprintf(buf, len + 1 - 3, "%s", string);
    assert_int_equal(written, len);

    assert_string_equal(buf, "toto_à");                   // Check the right string was copied
    assert_string_equal(&buf[len + 1 - 3], canary_mark);  // Check if Canary is still 'alive'

    // Last pass: write the full string minus 4 bytes => the last 2+3 bytes will be removed
    strcpy(&buf[len + 1 - 4], canary_mark);  // Write the Canary at the right location
    written = snprintf(buf, len + 1 - 4, "%s", string);
    assert_int_equal(written, len);

    assert_string_equal(buf, "toto_");                    // Check the right string was copied
    assert_string_equal(&buf[len + 1 - 4], canary_mark);  // Check if Canary is still 'alive'
}

// Check an UTF-8 string ending with a multi-bytes character using 4 bytes
static void test_snprintf_utf8_strings_4(void **state)
{
    UNUSED(state);
    char canary_mark[]
        = "Canary";  // Used to check snprintf didn't wrote any byte after the expected limit
    char string[] = "toto_à𐊶";  // toto_à + 0xF0 0x90 0x8A 0xB6

    char buf[sizeof(string) + sizeof(canary_mark)];  // sizeof take in account the '\0'
    int  written;

    size_t len = strlen(string);  // strlen returns the number of bytes without the ending '\0'
    assert_int_equal(len, 5 + 2 + 4);
    assert_int_equal(len + 1, sizeof(string));

    // First pass: write the full string
    strcpy(&buf[len + 1], canary_mark);  // Write the Canary at the right location
    written = snprintf(buf, len + 1, "%s", string);
    assert_int_equal(written, len);

    assert_string_equal(buf, string);                 // Check the full string was copied
    assert_string_equal(&buf[len + 1], canary_mark);  // Check if Canary is still 'alive'

    // Second pass: write the full string minus 1 byte => the last 3 bytes will be removed
    strcpy(&buf[len + 1 - 1], canary_mark);  // Write the Canary at the right location
    written = snprintf(buf, len + 1 - 1, "%s", string);
    assert_int_equal(written, len);

    assert_string_equal(buf, "toto_à");                   // Check the right string was copied
    assert_string_equal(&buf[len + 1 - 1], canary_mark);  // Check if Canary is still 'alive'

    // Third pass: write the full string minus 2 bytes => the last 3 bytes will be removed
    strcpy(&buf[len + 1 - 2], canary_mark);  // Write the Canary at the right location
    written = snprintf(buf, len + 1 - 2, "%s", string);
    assert_int_equal(written, len);

    assert_string_equal(buf, "toto_à");                   // Check the right string was copied
    assert_string_equal(&buf[len + 1 - 2], canary_mark);  // Check if Canary is still 'alive'

    // fourth pass: write the full string minus 3 bytes => the last 3 bytes will be removed
    strcpy(&buf[len + 1 - 3], canary_mark);  // Write the Canary at the right location
    written = snprintf(buf, len + 1 - 3, "%s", string);
    assert_int_equal(written, len);

    assert_string_equal(buf, "toto_à");                   // Check the right string was copied
    assert_string_equal(&buf[len + 1 - 3], canary_mark);  // Check if Canary is still 'alive'

    // Fifth pass: write the full string minus 4 bytes => the last 4 bytes will be removed
    strcpy(&buf[len + 1 - 4], canary_mark);  // Write the Canary at the right location
    written = snprintf(buf, len + 1 - 4, "%s", string);
    assert_int_equal(written, len);

    assert_string_equal(buf, "toto_à");                   // Check the right string was copied
    assert_string_equal(&buf[len + 1 - 4], canary_mark);  // Check if Canary is still 'alive'

    // Last pass: write the full string minus 5 bytes => the last 2+4 bytes will be removed
    strcpy(&buf[len + 1 - 5], canary_mark);  // Write the Canary at the right location
    written = snprintf(buf, len + 1 - 5, "%s", string);
    assert_int_equal(written, len);

    assert_string_equal(buf, "toto_");                    // Check the right string was copied
    assert_string_equal(&buf[len + 1 - 5], canary_mark);  // Check if Canary is still 'alive'
}

// ****************************************************************************
// Unit Tests for strlcpy with UTF-8 strings (based on the snprintf ones)
// ****************************************************************************

static void test_strlcpy_utf8_bad_params(void **state)
{
    char   buf[10];
    char   string[] = "toto_é";        // toto_ + 0xC3 0xA9 + '\0'
    size_t len      = strlen(string);  // strlen returns the number of bytes without the ending '\0'

    // src = NULL => strlcpy must return 0
    assert_int_equal(strlcpy_utf8(buf, NULL, sizeof(buf)), 0);

    // dstlen = 0 => strlcpy must return strlen(string)
    assert_int_equal(strlcpy_utf8(buf, string, 0), len);

    // dst = NULL => strlcpy must return strlen(string)
    assert_int_equal(strlcpy_utf8(NULL, string, sizeof(buf)), len);

    // make string wrong by removing one of the ending two UTF-8 bytes
    // despite non truncation occurred as strlen(string)+1 < sizeof(buf), buf will contain a valid
    // string
    string[len - 1] = '\0';  // toto_ + 0xC3 + '\0'
    assert_int_equal(strlcpy_utf8(buf, string, sizeof(buf)), strlen(string));
    assert_string_equal(buf, "toto_");  // Check the right string was copied
}

// Check an UTF-8 string ending with a multi-bytes character using 2 bytes
static void test_strlcpy_utf8_strings_2(void **state)
{
    UNUSED(state);
    char canary_mark[]
        = "Canary";  // Used to check strlcpy didn't wrote any byte after the expected limit
    char string[] = "toto_àé";  // toto_à + 0xC3 0xA9

    char buf[sizeof(string) + sizeof(canary_mark)];  // sizeof take in account the '\0'
    int  written;

    size_t len = strlen(string);  // strlen returns the number of bytes without the ending '\0'
    assert_int_equal(len, 5 + 2 + 2);
    assert_int_equal(len + 1, sizeof(string));

    // First pass: write the full string
    strcpy(&buf[len + 1], canary_mark);  // Write the Canary at the right location
    written = strlcpy_utf8(buf, string, len + 1);
    assert_int_equal(written, len);

    assert_string_equal(buf, string);                 // Check the full string was copied
    assert_string_equal(&buf[len + 1], canary_mark);  // Check if Canary is still 'alive'

    // Second pass: write the full string minus 1 byte => the last 2 bytes will be removed
    strcpy(&buf[len + 1 - 1], canary_mark);  // Write the Canary at the right location
    written = strlcpy_utf8(buf, string, len + 1 - 1);
    assert_int_equal(written, len);

    assert_string_equal(buf, "toto_à");                   // Check the right string was copied
    assert_string_equal(&buf[len + 1 - 1], canary_mark);  // Check if Canary is still 'alive'

    // Third pass: write the full string minus 2 bytes => the last 2 bytes will be removed
    strcpy(&buf[len + 1 - 2], canary_mark);  // Write the Canary at the right location
    written = strlcpy_utf8(buf, string, len + 1 - 2);
    assert_int_equal(written, len);

    assert_string_equal(buf, "toto_à");                   // Check the right string was copied
    assert_string_equal(&buf[len + 1 - 2], canary_mark);  // Check if Canary is still 'alive'

    // Last pass: write the full string minus 3 bytes => the last 2+2 bytes will be removed
    strcpy(&buf[len + 1 - 3], canary_mark);  // Write the Canary at the right location
    written = strlcpy_utf8(buf, string, len + 1 - 3);
    assert_int_equal(written, len);

    assert_string_equal(buf, "toto_");                    // Check the right string was copied
    assert_string_equal(&buf[len + 1 - 3], canary_mark);  // Check if Canary is still 'alive'
}

// Check an UTF-8 string ending with a multi-bytes character using 3 bytes
static void test_strlcpy_utf8_strings_3(void **state)
{
    UNUSED(state);
    char canary_mark[]
        = "Canary";  // Used to check strlcpy didn't wrote any byte after the expected limit
    char string[] = "toto_àด";  // toto_à + 0xE0 0xB8 0x94

    char buf[sizeof(string) + sizeof(canary_mark)];  // sizeof take in account the '\0'
    int  written;

    size_t len = strlen(string);  // strlen returns the number of bytes without the ending '\0'
    assert_int_equal(len, 5 + 2 + 3);
    assert_int_equal(len + 1, sizeof(string));

    // First pass: write the full string
    strcpy(&buf[len + 1], canary_mark);  // Write the Canary at the right location
    written = strlcpy_utf8(buf, string, len + 1);
    assert_int_equal(written, len);

    assert_string_equal(buf, string);                 // Check the full string was copied
    assert_string_equal(&buf[len + 1], canary_mark);  // Check if Canary is still 'alive'

    // Second pass: write the full string minus 1 byte => the last 3 bytes will be removed
    strcpy(&buf[len + 1 - 1], canary_mark);  // Write the Canary at the right location
    written = strlcpy_utf8(buf, string, len + 1 - 1);
    assert_int_equal(written, len);

    assert_string_equal(buf, "toto_à");                   // Check the right string was copied
    assert_string_equal(&buf[len + 1 - 1], canary_mark);  // Check if Canary is still 'alive'

    // Third pass: write the full string minus 2 bytes => the last 3 bytes will be removed
    strcpy(&buf[len + 1 - 2], canary_mark);  // Write the Canary at the right location
    written = strlcpy_utf8(buf, string, len + 1 - 2);
    assert_int_equal(written, len);

    assert_string_equal(buf, "toto_à");                   // Check the right string was copied
    assert_string_equal(&buf[len + 1 - 2], canary_mark);  // Check if Canary is still 'alive'

    // fourth pass: write the full string minus 3 bytes => the last 3 bytes will be removed
    strcpy(&buf[len + 1 - 3], canary_mark);  // Write the Canary at the right location
    written = strlcpy_utf8(buf, string, len + 1 - 3);
    assert_int_equal(written, len);

    assert_string_equal(buf, "toto_à");                   // Check the right string was copied
    assert_string_equal(&buf[len + 1 - 3], canary_mark);  // Check if Canary is still 'alive'

    // Last pass: write the full string minus 4 bytes => the last 2+3 bytes will be removed
    strcpy(&buf[len + 1 - 4], canary_mark);  // Write the Canary at the right location
    written = strlcpy_utf8(buf, string, len + 1 - 4);
    assert_int_equal(written, len);

    assert_string_equal(buf, "toto_");                    // Check the right string was copied
    assert_string_equal(&buf[len + 1 - 4], canary_mark);  // Check if Canary is still 'alive'
}

// Check an UTF-8 string ending with a multi-bytes character using 4 bytes
static void test_strlcpy_utf8_strings_4(void **state)
{
    UNUSED(state);
    char canary_mark[]
        = "Canary";  // Used to check strlcpy didn't wrote any byte after the expected limit
    char string[] = "toto_à𐊶";  // toto_à + 0xF0 0x90 0x8A 0xB6

    char buf[sizeof(string) + sizeof(canary_mark)];  // sizeof take in account the '\0'
    int  written;

    size_t len = strlen(string);  // strlen returns the number of bytes without the ending '\0'
    assert_int_equal(len, 5 + 2 + 4);
    assert_int_equal(len + 1, sizeof(string));

    // First pass: write the full string
    strcpy(&buf[len + 1], canary_mark);  // Write the Canary at the right location
    written = strlcpy_utf8(buf, string, len + 1);
    assert_int_equal(written, len);

    assert_string_equal(buf, string);                 // Check the full string was copied
    assert_string_equal(&buf[len + 1], canary_mark);  // Check if Canary is still 'alive'

    // Second pass: write the full string minus 1 byte => the last 3 bytes will be removed
    strcpy(&buf[len + 1 - 1], canary_mark);  // Write the Canary at the right location
    written = strlcpy_utf8(buf, string, len + 1 - 1);
    assert_int_equal(written, len);

    assert_string_equal(buf, "toto_à");                   // Check the right string was copied
    assert_string_equal(&buf[len + 1 - 1], canary_mark);  // Check if Canary is still 'alive'

    // Third pass: write the full string minus 2 bytes => the last 3 bytes will be removed
    strcpy(&buf[len + 1 - 2], canary_mark);  // Write the Canary at the right location
    written = strlcpy_utf8(buf, string, len + 1 - 2);
    assert_int_equal(written, len);

    assert_string_equal(buf, "toto_à");                   // Check the right string was copied
    assert_string_equal(&buf[len + 1 - 2], canary_mark);  // Check if Canary is still 'alive'

    // fourth pass: write the full string minus 3 bytes => the last 3 bytes will be removed
    strcpy(&buf[len + 1 - 3], canary_mark);  // Write the Canary at the right location
    written = strlcpy_utf8(buf, string, len + 1 - 3);
    assert_int_equal(written, len);

    assert_string_equal(buf, "toto_à");                   // Check the right string was copied
    assert_string_equal(&buf[len + 1 - 3], canary_mark);  // Check if Canary is still 'alive'

    // Fifth pass: write the full string minus 4 bytes => the last 4 bytes will be removed
    strcpy(&buf[len + 1 - 4], canary_mark);  // Write the Canary at the right location
    written = strlcpy_utf8(buf, string, len + 1 - 4);
    assert_int_equal(written, len);

    assert_string_equal(buf, "toto_à");                   // Check the right string was copied
    assert_string_equal(&buf[len + 1 - 4], canary_mark);  // Check if Canary is still 'alive'

    // Last pass: write the full string minus 5 bytes => the last 2+4 bytes will be removed
    strcpy(&buf[len + 1 - 5], canary_mark);  // Write the Canary at the right location
    written = strlcpy_utf8(buf, string, len + 1 - 5);
    assert_int_equal(written, len);

    assert_string_equal(buf, "toto_");                    // Check the right string was copied
    assert_string_equal(&buf[len + 1 - 5], canary_mark);  // Check if Canary is still 'alive'
}

// ****************************************************************************
// Main Unit Tests
// ****************************************************************************

int main(int argc, char **argv)
{
    const struct CMUnitTest tests[]
        = {// PRINTF tests
           cmocka_unit_test_setup(test_printf_llu, setup),
           cmocka_unit_test_setup(test_printf_lld, setup),
           cmocka_unit_test_setup(test_printf_llx, setup),
           cmocka_unit_test_setup(test_printf_llX_padded, setup),
           cmocka_unit_test_setup(test_printf_long_padding, setup),
           cmocka_unit_test_setup(test_printf_short_padding, setup),
           cmocka_unit_test_setup(test_printf_d_padded, setup),
           // snprintf tests
           cmocka_unit_test_setup(test_snprintf_lld, setup),
           cmocka_unit_test_setup(test_snprintf_llX_padded, setup),
           cmocka_unit_test_setup(test_snprintf_small_buffer, setup),
           // snprintf utf-8 tests
           cmocka_unit_test_setup(test_snprintf_utf8_small_buffer, setup),
           cmocka_unit_test_setup(test_snprintf_utf8_strings_2, setup),
           cmocka_unit_test_setup(test_snprintf_utf8_strings_3, setup),
           cmocka_unit_test_setup(test_snprintf_utf8_strings_4, setup),
           // strlcpy utf-8 tests
           cmocka_unit_test_setup(test_strlcpy_utf8_bad_params, setup),
           cmocka_unit_test_setup(test_strlcpy_utf8_strings_2, setup),
           cmocka_unit_test_setup(test_strlcpy_utf8_strings_3, setup),
           cmocka_unit_test_setup(test_strlcpy_utf8_strings_4, setup)};
    return cmocka_run_group_tests(tests, NULL, NULL);
}
