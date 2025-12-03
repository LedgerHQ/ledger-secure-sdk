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
    // snprintf should return the number of characters that would have been written
    // "Hello" = 5 characters (excluding '\0')
    assert_int_equal(written, 5);
}

// ****************************************************************************
// Main Unit Tests
// ****************************************************************************

int main(int argc, char **argv)
{
    const struct CMUnitTest tests[] = {
        // PRINTF tests
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
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
