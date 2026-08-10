#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

#include "unity.h"
#include "Mockos_io_seph_cmd.h"

#include "os_print.h"
#include "os_helpers.h"
#include "os_utils.h"

static char   output_buffer[1024];
static size_t output_pos = 0;

static void capture_printf(const char *data, uint16_t len, int num_calls)
{
    (void) num_calls;
    if (output_pos + len < sizeof(output_buffer)) {
        memcpy(output_buffer + output_pos, data, len);
        output_pos += len;
        output_buffer[output_pos] = '\0';
    }
}

void setUp(void)
{
    Mockos_io_seph_cmd_Init();
    os_io_seph_cmd_printf_StubWithCallback(capture_printf);
    memset(output_buffer, 0, sizeof(output_buffer));
    output_pos = 0;
}

void tearDown(void)
{
    Mockos_io_seph_cmd_Verify();
    Mockos_io_seph_cmd_Destroy();
}

// ****************************************************************************
// Unit Tests for mcu_usb_printf (PRINTF)
// ****************************************************************************

// Test printf with %llu
void test_printf_llu(void)
{
    uint64_t value = 9876543210123456789ULL;

    mcu_usb_printf("(DEC) unsigned_value: %llu", value);
    TEST_ASSERT_EQUAL_STRING("(DEC) unsigned_value: 9876543210123456789", output_buffer);
}

// Test printf with %lld
void test_printf_lld(void)
{
    int64_t value = -123456789012345LL;

    mcu_usb_printf("(DEC) signed_value: %lld", value);
    TEST_ASSERT_EQUAL_STRING("(DEC) signed_value: -123456789012345", output_buffer);
}

// Test printf with %llx
void test_printf_llx(void)
{
    uint64_t value = 0x123456789ABCDEF0ULL;

    mcu_usb_printf("(HEX) value: 0x%llx", value);
    TEST_ASSERT_EQUAL_STRING("(HEX) value: 0x123456789abcdef0", output_buffer);
}

// Test printf with %llX with padding
void test_printf_llX_padded(void)
{
    uint64_t value = 0x123456789ABCDEF0ULL;

    mcu_usb_printf("(HEX) value: 0x%020llX", value);
    TEST_ASSERT_EQUAL_STRING("(HEX) value: 0x0000123456789ABCDEF0", output_buffer);
}

// Test printf with %d with padding
void test_printf_d_padded(void)
{
    int32_t value = 4662;

    mcu_usb_printf("Padding: '%25d'", value);
    TEST_ASSERT_EQUAL_STRING("Padding: '                     4662'", output_buffer);
}

// Test printf with long padding
void test_printf_long_padding(void)
{
    int64_t value = 4661ULL;

    mcu_usb_printf("Padding: '%67lld'", value);
    TEST_ASSERT_EQUAL_STRING(
        "Padding: '                                                               4661'",
        output_buffer);
}

// Test printf with short padding
void test_printf_short_padding(void)
{
    int64_t value = 123454661ULL;

    mcu_usb_printf("Padding: '%5lld'", value);
    TEST_ASSERT_EQUAL_STRING("Padding: '123454661'", output_buffer);
}

// ****************************************************************************
// Unit Tests for snprintf
// ****************************************************************************

// Test snprintf with %lld
void test_snprintf_lld(void)
{
    int64_t value      = -123456789012345LL;
    char    expected[] = "Dec: -123456789012345";
    int     written;

    // use cast because on x86 platform, type can be different (in default standard headers)
    written = snprintf(output_buffer, sizeof(output_buffer), "Dec: %lld", (long long) value);
    TEST_ASSERT_EQUAL_STRING(expected, output_buffer);
    TEST_ASSERT_EQUAL_INT(strlen(expected), written);
}

// Test snprintf with %llX with padding
void test_snprintf_llX_padded(void)
{
    uint64_t value      = 0x123456789ABCDEF0ULL;
    char     expected[] = "Hex: 0x123456789ABCDEF0";
    int      written;

    // use cast because on x86 platform, type can be different (in default standard headers)
    written = snprintf(
        output_buffer, sizeof(output_buffer), "Hex: 0x%016llX", (unsigned long long) value);
    TEST_ASSERT_EQUAL_STRING(expected, output_buffer);
    TEST_ASSERT_EQUAL_INT(strlen(expected), written);
}

// Test snprintf with small buffer
void test_snprintf_small_buffer(void)
{
    char buf[1];
    int  written;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
    written = snprintf(buf, 1, "Hello");
#pragma GCC diagnostic pop
    TEST_ASSERT_EQUAL_STRING("", buf);  // Buffer should only contain '\0'
    // snprintf should return the number of bytes that would have been written
    // "Hello" = 5 bytes (excluding '\0')
    TEST_ASSERT_EQUAL_INT(5, written);
}

// ****************************************************************************
// Unit Tests for snprintf with UTF-8 strings
// ****************************************************************************

// Test snprintf with small buffer
void test_snprintf_utf8_small_buffer(void)
{
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
    TEST_ASSERT_EQUAL_STRING("é", buf);                  // Buffer should only contain 'é'
    TEST_ASSERT_EQUAL_STRING(canary_mark, &buf[2 + 1]);  // Check if Canary is still 'alive'
    // snprintf should return the number of bytes that would have been written
    // "été" = 5 bytes (excluding '\0')
    TEST_ASSERT_EQUAL_INT(5, written);

    // Same, but with only 1 byte => the UTF-8 character 'é' will be lost
    // Write the Canary
    strcpy(&buf[1 + 1], canary_mark);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
    written = snprintf(buf, 1 + 1, "été");  // 0xC3 0xA9 0x74 0xC3 0xA9 0x00
#pragma GCC diagnostic pop
    // Improved snprintf, who doesn't cut a utf-8 character
    TEST_ASSERT_EQUAL_INT(0,
                          (uint8_t) buf[0]);  // The invalid 0xC3 character was replaced by a '\0'
    TEST_ASSERT_EQUAL_INT(0x00, buf[1]);      // '\0'
    TEST_ASSERT_EQUAL_STRING(canary_mark, &buf[1 + 1]);  // Check if Canary is still 'alive'
    // snprintf should return the number of bytes that would have been written
    // "été" = 5 bytes (excluding '\0')
    TEST_ASSERT_EQUAL_INT(5, written);
}

// Check an UTF-8 string ending with a multi-bytes character using 2 bytes
void test_snprintf_utf8_strings_2(void)
{
    char canary_mark[]
        = "Canary";  // Used to check snprintf didn't wrote any byte after the expected limit
    char string[] = "toto_àé";  // toto_à + 0xC3 0xA9

    char buf[sizeof(string) + sizeof(canary_mark)];  // sizeof take in account the '\0'
    int  written;

    size_t len = strlen(string);  // strlen returns the number of bytes without the ending '\0'
    TEST_ASSERT_EQUAL_INT(5 + 2 + 2, len);
    TEST_ASSERT_EQUAL_INT(sizeof(string), len + 1);

    // First pass: write the full string
    strcpy(&buf[len + 1], canary_mark);  // Write the Canary at the right location
    written = snprintf(buf, len + 1, "%s", string);
    TEST_ASSERT_EQUAL_INT(len, written);

    TEST_ASSERT_EQUAL_STRING(string, buf);                 // Check the full string was copied
    TEST_ASSERT_EQUAL_STRING(canary_mark, &buf[len + 1]);  // Check if Canary is still 'alive'

    // Second pass: write the full string minus 1 byte => the last 2 bytes will be removed
    strcpy(&buf[len + 1 - 1], canary_mark);  // Write the Canary at the right location
    written = snprintf(buf, len + 1 - 1, "%s", string);
    TEST_ASSERT_EQUAL_INT(len, written);

    TEST_ASSERT_EQUAL_STRING("toto_à", buf);                   // Check the right string was copied
    TEST_ASSERT_EQUAL_STRING(canary_mark, &buf[len + 1 - 1]);  // Check if Canary is still 'alive'

    // Third pass: write the full string minus 2 bytes => the last 2 bytes will be removed
    strcpy(&buf[len + 1 - 2], canary_mark);  // Write the Canary at the right location
    written = snprintf(buf, len + 1 - 2, "%s", string);
    TEST_ASSERT_EQUAL_INT(len, written);

    TEST_ASSERT_EQUAL_STRING("toto_à", buf);                   // Check the right string was copied
    TEST_ASSERT_EQUAL_STRING(canary_mark, &buf[len + 1 - 2]);  // Check if Canary is still 'alive'

    // Last pass: write the full string minus 3 bytes => the last 2+2 bytes will be removed
    strcpy(&buf[len + 1 - 3], canary_mark);  // Write the Canary at the right location
    written = snprintf(buf, len + 1 - 3, "%s", string);
    TEST_ASSERT_EQUAL_INT(len, written);

    TEST_ASSERT_EQUAL_STRING("toto_", buf);                    // Check the right string was copied
    TEST_ASSERT_EQUAL_STRING(canary_mark, &buf[len + 1 - 3]);  // Check if Canary is still 'alive'
}

// Check an UTF-8 string ending with a multi-bytes character using 3 bytes
void test_snprintf_utf8_strings_3(void)
{
    char canary_mark[]
        = "Canary";  // Used to check snprintf didn't wrote any byte after the expected limit
    char string[] = "toto_àด";  // toto_à + 0xE0 0xB8 0x94

    char buf[sizeof(string) + sizeof(canary_mark)];  // sizeof take in account the '\0'
    int  written;

    size_t len = strlen(string);  // strlen returns the number of bytes without the ending '\0'
    TEST_ASSERT_EQUAL_INT(5 + 2 + 3, len);
    TEST_ASSERT_EQUAL_INT(sizeof(string), len + 1);

    // First pass: write the full string
    strcpy(&buf[len + 1], canary_mark);  // Write the Canary at the right location
    written = snprintf(buf, len + 1, "%s", string);
    TEST_ASSERT_EQUAL_INT(len, written);

    TEST_ASSERT_EQUAL_STRING(string, buf);                 // Check the full string was copied
    TEST_ASSERT_EQUAL_STRING(canary_mark, &buf[len + 1]);  // Check if Canary is still 'alive'

    // Second pass: write the full string minus 1 byte => the last 3 bytes will be removed
    strcpy(&buf[len + 1 - 1], canary_mark);  // Write the Canary at the right location
    written = snprintf(buf, len + 1 - 1, "%s", string);
    TEST_ASSERT_EQUAL_INT(len, written);

    TEST_ASSERT_EQUAL_STRING("toto_à", buf);                   // Check the right string was copied
    TEST_ASSERT_EQUAL_STRING(canary_mark, &buf[len + 1 - 1]);  // Check if Canary is still 'alive'

    // Third pass: write the full string minus 2 bytes => the last 3 bytes will be removed
    strcpy(&buf[len + 1 - 2], canary_mark);  // Write the Canary at the right location
    written = snprintf(buf, len + 1 - 2, "%s", string);
    TEST_ASSERT_EQUAL_INT(len, written);

    TEST_ASSERT_EQUAL_STRING("toto_à", buf);                   // Check the right string was copied
    TEST_ASSERT_EQUAL_STRING(canary_mark, &buf[len + 1 - 2]);  // Check if Canary is still 'alive'

    // fourth pass: write the full string minus 3 bytes => the last 3 bytes will be removed
    strcpy(&buf[len + 1 - 3], canary_mark);  // Write the Canary at the right location
    written = snprintf(buf, len + 1 - 3, "%s", string);
    TEST_ASSERT_EQUAL_INT(len, written);

    TEST_ASSERT_EQUAL_STRING("toto_à", buf);                   // Check the right string was copied
    TEST_ASSERT_EQUAL_STRING(canary_mark, &buf[len + 1 - 3]);  // Check if Canary is still 'alive'

    // Last pass: write the full string minus 4 bytes => the last 2+3 bytes will be removed
    strcpy(&buf[len + 1 - 4], canary_mark);  // Write the Canary at the right location
    written = snprintf(buf, len + 1 - 4, "%s", string);
    TEST_ASSERT_EQUAL_INT(len, written);

    TEST_ASSERT_EQUAL_STRING("toto_", buf);                    // Check the right string was copied
    TEST_ASSERT_EQUAL_STRING(canary_mark, &buf[len + 1 - 4]);  // Check if Canary is still 'alive'
}

// Check an UTF-8 string ending with a multi-bytes character using 4 bytes
void test_snprintf_utf8_strings_4(void)
{
    char canary_mark[]
        = "Canary";  // Used to check snprintf didn't wrote any byte after the expected limit
    char string[] = "toto_à𐊶";  // toto_à + 0xF0 0x90 0x8A 0xB6

    char buf[sizeof(string) + sizeof(canary_mark)];  // sizeof take in account the '\0'
    int  written;

    size_t len = strlen(string);  // strlen returns the number of bytes without the ending '\0'
    TEST_ASSERT_EQUAL_INT(5 + 2 + 4, len);
    TEST_ASSERT_EQUAL_INT(sizeof(string), len + 1);

    // First pass: write the full string
    strcpy(&buf[len + 1], canary_mark);  // Write the Canary at the right location
    written = snprintf(buf, len + 1, "%s", string);
    TEST_ASSERT_EQUAL_INT(len, written);

    TEST_ASSERT_EQUAL_STRING(string, buf);                 // Check the full string was copied
    TEST_ASSERT_EQUAL_STRING(canary_mark, &buf[len + 1]);  // Check if Canary is still 'alive'

    // Second pass: write the full string minus 1 byte => the last 3 bytes will be removed
    strcpy(&buf[len + 1 - 1], canary_mark);  // Write the Canary at the right location
    written = snprintf(buf, len + 1 - 1, "%s", string);
    TEST_ASSERT_EQUAL_INT(len, written);

    TEST_ASSERT_EQUAL_STRING("toto_à", buf);                   // Check the right string was copied
    TEST_ASSERT_EQUAL_STRING(canary_mark, &buf[len + 1 - 1]);  // Check if Canary is still 'alive'

    // Third pass: write the full string minus 2 bytes => the last 3 bytes will be removed
    strcpy(&buf[len + 1 - 2], canary_mark);  // Write the Canary at the right location
    written = snprintf(buf, len + 1 - 2, "%s", string);
    TEST_ASSERT_EQUAL_INT(len, written);

    TEST_ASSERT_EQUAL_STRING("toto_à", buf);                   // Check the right string was copied
    TEST_ASSERT_EQUAL_STRING(canary_mark, &buf[len + 1 - 2]);  // Check if Canary is still 'alive'

    // fourth pass: write the full string minus 3 bytes => the last 3 bytes will be removed
    strcpy(&buf[len + 1 - 3], canary_mark);  // Write the Canary at the right location
    written = snprintf(buf, len + 1 - 3, "%s", string);
    TEST_ASSERT_EQUAL_INT(len, written);

    TEST_ASSERT_EQUAL_STRING("toto_à", buf);                   // Check the right string was copied
    TEST_ASSERT_EQUAL_STRING(canary_mark, &buf[len + 1 - 3]);  // Check if Canary is still 'alive'

    // Fifth pass: write the full string minus 4 bytes => the last 4 bytes will be removed
    strcpy(&buf[len + 1 - 4], canary_mark);  // Write the Canary at the right location
    written = snprintf(buf, len + 1 - 4, "%s", string);
    TEST_ASSERT_EQUAL_INT(len, written);

    TEST_ASSERT_EQUAL_STRING("toto_à", buf);                   // Check the right string was copied
    TEST_ASSERT_EQUAL_STRING(canary_mark, &buf[len + 1 - 4]);  // Check if Canary is still 'alive'

    // Last pass: write the full string minus 5 bytes => the last 2+4 bytes will be removed
    strcpy(&buf[len + 1 - 5], canary_mark);  // Write the Canary at the right location
    written = snprintf(buf, len + 1 - 5, "%s", string);
    TEST_ASSERT_EQUAL_INT(len, written);

    TEST_ASSERT_EQUAL_STRING("toto_", buf);                    // Check the right string was copied
    TEST_ASSERT_EQUAL_STRING(canary_mark, &buf[len + 1 - 5]);  // Check if Canary is still 'alive'
}

// ****************************************************************************
// Unit Tests for strlcpy with UTF-8 strings (based on the snprintf ones)
// ****************************************************************************

void test_strlcpy_utf8_bad_params(void)
{
    char   buf[10];
    char   string[] = "toto_é";        // toto_ + 0xC3 0xA9 + '\0'
    size_t len      = strlen(string);  // strlen returns the number of bytes without the ending '\0'

    // src = NULL => strlcpy must return 0
    TEST_ASSERT_EQUAL_INT(0, strlcpy_utf8(buf, NULL, sizeof(buf)));

    // dstlen = 0 => strlcpy must return strlen(string)
    TEST_ASSERT_EQUAL_INT(len, strlcpy_utf8(buf, string, 0));

    // dst = NULL => strlcpy must return strlen(string)
    TEST_ASSERT_EQUAL_INT(len, strlcpy_utf8(NULL, string, sizeof(buf)));

    // make string wrong by removing one of the ending two UTF-8 bytes
    // despite non truncation occurred as strlen(string)+1 < sizeof(buf), buf will contain a valid
    // string
    string[len - 1] = '\0';  // toto_ + 0xC3 + '\0'
    TEST_ASSERT_EQUAL_INT(strlen(string), strlcpy_utf8(buf, string, sizeof(buf)));
    TEST_ASSERT_EQUAL_STRING("toto_", buf);  // Check the right string was copied
}

// Check an UTF-8 string ending with a multi-bytes character using 2 bytes
void test_strlcpy_utf8_strings_2(void)
{
    char canary_mark[]
        = "Canary";  // Used to check strlcpy didn't wrote any byte after the expected limit
    char string[] = "toto_àé";  // toto_à + 0xC3 0xA9

    char buf[sizeof(string) + sizeof(canary_mark)];  // sizeof take in account the '\0'
    int  written;

    size_t len = strlen(string);  // strlen returns the number of bytes without the ending '\0'
    TEST_ASSERT_EQUAL_INT(5 + 2 + 2, len);
    TEST_ASSERT_EQUAL_INT(sizeof(string), len + 1);

    // First pass: write the full string
    strcpy(&buf[len + 1], canary_mark);  // Write the Canary at the right location
    written = strlcpy_utf8(buf, string, len + 1);
    TEST_ASSERT_EQUAL_INT(len, written);

    TEST_ASSERT_EQUAL_STRING(string, buf);                 // Check the full string was copied
    TEST_ASSERT_EQUAL_STRING(canary_mark, &buf[len + 1]);  // Check if Canary is still 'alive'

    // Second pass: write the full string minus 1 byte => the last 2 bytes will be removed
    strcpy(&buf[len + 1 - 1], canary_mark);  // Write the Canary at the right location
    written = strlcpy_utf8(buf, string, len + 1 - 1);
    TEST_ASSERT_EQUAL_INT(len, written);

    TEST_ASSERT_EQUAL_STRING("toto_à", buf);                   // Check the right string was copied
    TEST_ASSERT_EQUAL_STRING(canary_mark, &buf[len + 1 - 1]);  // Check if Canary is still 'alive'

    // Third pass: write the full string minus 2 bytes => the last 2 bytes will be removed
    strcpy(&buf[len + 1 - 2], canary_mark);  // Write the Canary at the right location
    written = strlcpy_utf8(buf, string, len + 1 - 2);
    TEST_ASSERT_EQUAL_INT(len, written);

    TEST_ASSERT_EQUAL_STRING("toto_à", buf);                   // Check the right string was copied
    TEST_ASSERT_EQUAL_STRING(canary_mark, &buf[len + 1 - 2]);  // Check if Canary is still 'alive'

    // Last pass: write the full string minus 3 bytes => the last 2+2 bytes will be removed
    strcpy(&buf[len + 1 - 3], canary_mark);  // Write the Canary at the right location
    written = strlcpy_utf8(buf, string, len + 1 - 3);
    TEST_ASSERT_EQUAL_INT(len, written);

    TEST_ASSERT_EQUAL_STRING("toto_", buf);                    // Check the right string was copied
    TEST_ASSERT_EQUAL_STRING(canary_mark, &buf[len + 1 - 3]);  // Check if Canary is still 'alive'
}

// Check an UTF-8 string ending with a multi-bytes character using 3 bytes
void test_strlcpy_utf8_strings_3(void)
{
    char canary_mark[]
        = "Canary";  // Used to check strlcpy didn't wrote any byte after the expected limit
    char string[] = "toto_àด";  // toto_à + 0xE0 0xB8 0x94

    char buf[sizeof(string) + sizeof(canary_mark)];  // sizeof take in account the '\0'
    int  written;

    size_t len = strlen(string);  // strlen returns the number of bytes without the ending '\0'
    TEST_ASSERT_EQUAL_INT(5 + 2 + 3, len);
    TEST_ASSERT_EQUAL_INT(sizeof(string), len + 1);

    // First pass: write the full string
    strcpy(&buf[len + 1], canary_mark);  // Write the Canary at the right location
    written = strlcpy_utf8(buf, string, len + 1);
    TEST_ASSERT_EQUAL_INT(len, written);

    TEST_ASSERT_EQUAL_STRING(string, buf);                 // Check the full string was copied
    TEST_ASSERT_EQUAL_STRING(canary_mark, &buf[len + 1]);  // Check if Canary is still 'alive'

    // Second pass: write the full string minus 1 byte => the last 3 bytes will be removed
    strcpy(&buf[len + 1 - 1], canary_mark);  // Write the Canary at the right location
    written = strlcpy_utf8(buf, string, len + 1 - 1);
    TEST_ASSERT_EQUAL_INT(len, written);

    TEST_ASSERT_EQUAL_STRING("toto_à", buf);                   // Check the right string was copied
    TEST_ASSERT_EQUAL_STRING(canary_mark, &buf[len + 1 - 1]);  // Check if Canary is still 'alive'

    // Third pass: write the full string minus 2 bytes => the last 3 bytes will be removed
    strcpy(&buf[len + 1 - 2], canary_mark);  // Write the Canary at the right location
    written = strlcpy_utf8(buf, string, len + 1 - 2);
    TEST_ASSERT_EQUAL_INT(len, written);

    TEST_ASSERT_EQUAL_STRING("toto_à", buf);                   // Check the right string was copied
    TEST_ASSERT_EQUAL_STRING(canary_mark, &buf[len + 1 - 2]);  // Check if Canary is still 'alive'

    // fourth pass: write the full string minus 3 bytes => the last 3 bytes will be removed
    strcpy(&buf[len + 1 - 3], canary_mark);  // Write the Canary at the right location
    written = strlcpy_utf8(buf, string, len + 1 - 3);
    TEST_ASSERT_EQUAL_INT(len, written);

    TEST_ASSERT_EQUAL_STRING("toto_à", buf);                   // Check the right string was copied
    TEST_ASSERT_EQUAL_STRING(canary_mark, &buf[len + 1 - 3]);  // Check if Canary is still 'alive'

    // Last pass: write the full string minus 4 bytes => the last 2+3 bytes will be removed
    strcpy(&buf[len + 1 - 4], canary_mark);  // Write the Canary at the right location
    written = strlcpy_utf8(buf, string, len + 1 - 4);
    TEST_ASSERT_EQUAL_INT(len, written);

    TEST_ASSERT_EQUAL_STRING("toto_", buf);                    // Check the right string was copied
    TEST_ASSERT_EQUAL_STRING(canary_mark, &buf[len + 1 - 4]);  // Check if Canary is still 'alive'
}

// Check an UTF-8 string ending with a multi-bytes character using 4 bytes
void test_strlcpy_utf8_strings_4(void)
{
    char canary_mark[]
        = "Canary";  // Used to check strlcpy didn't wrote any byte after the expected limit
    char string[] = "toto_à𐊶";  // toto_à + 0xF0 0x90 0x8A 0xB6

    char buf[sizeof(string) + sizeof(canary_mark)];  // sizeof take in account the '\0'
    int  written;

    size_t len = strlen(string);  // strlen returns the number of bytes without the ending '\0'
    TEST_ASSERT_EQUAL_INT(5 + 2 + 4, len);
    TEST_ASSERT_EQUAL_INT(sizeof(string), len + 1);

    // First pass: write the full string
    strcpy(&buf[len + 1], canary_mark);  // Write the Canary at the right location
    written = strlcpy_utf8(buf, string, len + 1);
    TEST_ASSERT_EQUAL_INT(len, written);

    TEST_ASSERT_EQUAL_STRING(string, buf);                 // Check the full string was copied
    TEST_ASSERT_EQUAL_STRING(canary_mark, &buf[len + 1]);  // Check if Canary is still 'alive'

    // Second pass: write the full string minus 1 byte => the last 3 bytes will be removed
    strcpy(&buf[len + 1 - 1], canary_mark);  // Write the Canary at the right location
    written = strlcpy_utf8(buf, string, len + 1 - 1);
    TEST_ASSERT_EQUAL_INT(len, written);

    TEST_ASSERT_EQUAL_STRING("toto_à", buf);                   // Check the right string was copied
    TEST_ASSERT_EQUAL_STRING(canary_mark, &buf[len + 1 - 1]);  // Check if Canary is still 'alive'

    // Third pass: write the full string minus 2 bytes => the last 3 bytes will be removed
    strcpy(&buf[len + 1 - 2], canary_mark);  // Write the Canary at the right location
    written = strlcpy_utf8(buf, string, len + 1 - 2);
    TEST_ASSERT_EQUAL_INT(len, written);

    TEST_ASSERT_EQUAL_STRING("toto_à", buf);                   // Check the right string was copied
    TEST_ASSERT_EQUAL_STRING(canary_mark, &buf[len + 1 - 2]);  // Check if Canary is still 'alive'

    // fourth pass: write the full string minus 3 bytes => the last 3 bytes will be removed
    strcpy(&buf[len + 1 - 3], canary_mark);  // Write the Canary at the right location
    written = strlcpy_utf8(buf, string, len + 1 - 3);
    TEST_ASSERT_EQUAL_INT(len, written);

    TEST_ASSERT_EQUAL_STRING("toto_à", buf);                   // Check the right string was copied
    TEST_ASSERT_EQUAL_STRING(canary_mark, &buf[len + 1 - 3]);  // Check if Canary is still 'alive'

    // Fifth pass: write the full string minus 4 bytes => the last 4 bytes will be removed
    strcpy(&buf[len + 1 - 4], canary_mark);  // Write the Canary at the right location
    written = strlcpy_utf8(buf, string, len + 1 - 4);
    TEST_ASSERT_EQUAL_INT(len, written);

    TEST_ASSERT_EQUAL_STRING("toto_à", buf);                   // Check the right string was copied
    TEST_ASSERT_EQUAL_STRING(canary_mark, &buf[len + 1 - 4]);  // Check if Canary is still 'alive'

    // Last pass: write the full string minus 5 bytes => the last 2+4 bytes will be removed
    strcpy(&buf[len + 1 - 5], canary_mark);  // Write the Canary at the right location
    written = strlcpy_utf8(buf, string, len + 1 - 5);
    TEST_ASSERT_EQUAL_INT(len, written);

    TEST_ASSERT_EQUAL_STRING("toto_", buf);                    // Check the right string was copied
    TEST_ASSERT_EQUAL_STRING(canary_mark, &buf[len + 1 - 5]);  // Check if Canary is still 'alive'
}

// ****************************************************************************
// Main Unit Tests
// ****************************************************************************

int main(void)
{
    UNITY_BEGIN();
    // PRINTF tests
    RUN_TEST(test_printf_llu);
    RUN_TEST(test_printf_lld);
    RUN_TEST(test_printf_llx);
    RUN_TEST(test_printf_llX_padded);
    RUN_TEST(test_printf_long_padding);
    RUN_TEST(test_printf_short_padding);
    RUN_TEST(test_printf_d_padded);
    // snprintf tests
    RUN_TEST(test_snprintf_lld);
    RUN_TEST(test_snprintf_llX_padded);
    RUN_TEST(test_snprintf_small_buffer);
    // snprintf utf-8 tests
    RUN_TEST(test_snprintf_utf8_small_buffer);
    RUN_TEST(test_snprintf_utf8_strings_2);
    RUN_TEST(test_snprintf_utf8_strings_3);
    RUN_TEST(test_snprintf_utf8_strings_4);
    // strlcpy utf-8 tests
    RUN_TEST(test_strlcpy_utf8_bad_params);
    RUN_TEST(test_strlcpy_utf8_strings_2);
    RUN_TEST(test_strlcpy_utf8_strings_3);
    RUN_TEST(test_strlcpy_utf8_strings_4);
    return UNITY_END();
}
