#include <stdint.h>
#include <string.h>

#include "unity.h"

#include "os_utils.h"

static const uint8_t BYTES[] = {0x01, 0xAB, 0xFF};

void test_bytes_to_hex(void)
{
    char out[7];

    TEST_ASSERT_EQUAL_INT(0, bytes_to_hex(out, sizeof(out), BYTES, sizeof(BYTES)));
    TEST_ASSERT_EQUAL_STRING("01ABFF", out);
    TEST_ASSERT_EQUAL_INT(0, bytes_to_lowercase_hex(out, sizeof(out), BYTES, sizeof(BYTES)));
    TEST_ASSERT_EQUAL_STRING("01abff", out);
}

void test_bytes_to_hex_empty_input(void)
{
    char out[1] = {'X'};

    TEST_ASSERT_EQUAL_INT(0, bytes_to_hex(out, sizeof(out), BYTES, 0));
    TEST_ASSERT_EQUAL_STRING("", out);
}

void test_bytes_to_hex_buffer_too_small(void)
{
    char out[8];

    memset(out, 'X', sizeof(out));
    TEST_ASSERT_EQUAL_INT(-1, bytes_to_hex(out, 6, BYTES, sizeof(BYTES)));
    TEST_ASSERT_EQUAL_STRING("", out);
    TEST_ASSERT_EQUAL_INT(-1, bytes_to_lowercase_hex(out, 6, BYTES, sizeof(BYTES)));
    TEST_ASSERT_EQUAL_STRING("", out);
}

void test_bytes_to_hex_zero_size_buffer_untouched(void)
{
    char out = 'X';

    TEST_ASSERT_EQUAL_INT(-1, bytes_to_hex(&out, 0, BYTES, sizeof(BYTES)));
    TEST_ASSERT_EQUAL_CHAR('X', out);
    TEST_ASSERT_EQUAL_INT(-1, bytes_to_lowercase_hex(&out, 0, BYTES, 0));
    TEST_ASSERT_EQUAL_CHAR('X', out);
}

void test_bytes_to_hex_huge_length_rejected(void)
{
    char out[8];

    // 2 * len + 1 would wrap around
    TEST_ASSERT_EQUAL_INT(-1, bytes_to_hex(out, sizeof(out), BYTES, (SIZE_MAX / 2) + 1));
    TEST_ASSERT_EQUAL_STRING("", out);
}

void test_hex_string_sizes(void)
{
    TEST_ASSERT_EQUAL_UINT(6, HEX_STRING_LEN(sizeof(BYTES)));
    TEST_ASSERT_EQUAL_UINT(7, HEX_STRING_SIZE(sizeof(BYTES)));
    TEST_ASSERT_EQUAL_UINT(9, HEX_0X_STRING_SIZE(sizeof(BYTES)));
    // Arguments are evaluated as a whole
    TEST_ASSERT_EQUAL_UINT(10, HEX_STRING_LEN(2 + 3));
}

void test_bytes_to_hex_0x(void)
{
    char out[HEX_0X_STRING_SIZE(sizeof(BYTES))];

    TEST_ASSERT_EQUAL_INT(0, bytes_to_hex_0x(out, sizeof(out), BYTES, sizeof(BYTES)));
    TEST_ASSERT_EQUAL_STRING("0x01ABFF", out);
    TEST_ASSERT_EQUAL_INT(0, bytes_to_lowercase_hex_0x(out, sizeof(out), BYTES, sizeof(BYTES)));
    TEST_ASSERT_EQUAL_STRING("0x01abff", out);
}

void test_bytes_to_hex_0x_empty_input(void)
{
    char out[HEX_0X_STRING_SIZE(0)];

    TEST_ASSERT_EQUAL_INT(0, bytes_to_hex_0x(out, sizeof(out), BYTES, 0));
    TEST_ASSERT_EQUAL_STRING("0x", out);
}

void test_bytes_to_hex_0x_buffer_too_small(void)
{
    char out[HEX_0X_STRING_SIZE(sizeof(BYTES))];

    for (size_t outl = 1; outl < sizeof(out); outl++) {
        memset(out, 'X', sizeof(out));
        TEST_ASSERT_EQUAL_INT(-1, bytes_to_hex_0x(out, outl, BYTES, sizeof(BYTES)));
        TEST_ASSERT_EQUAL_STRING("", out);
        TEST_ASSERT_EQUAL_INT(-1, bytes_to_lowercase_hex_0x(out, outl, BYTES, sizeof(BYTES)));
        TEST_ASSERT_EQUAL_STRING("", out);
    }
}

void test_bytes_to_hex_0x_zero_size_buffer_untouched(void)
{
    char out = 'X';

    TEST_ASSERT_EQUAL_INT(-1, bytes_to_hex_0x(&out, 0, BYTES, 0));
    TEST_ASSERT_EQUAL_CHAR('X', out);
}

void test_bytes_to_hex_null_pointers(void)
{
    char out[HEX_0X_STRING_SIZE(sizeof(BYTES))];

    TEST_ASSERT_EQUAL_INT(-1, bytes_to_hex(NULL, sizeof(out), BYTES, sizeof(BYTES)));
    TEST_ASSERT_EQUAL_INT(-1, bytes_to_hex_0x(NULL, sizeof(out), BYTES, sizeof(BYTES)));

    memset(out, 'X', sizeof(out));
    TEST_ASSERT_EQUAL_INT(-1, bytes_to_hex(out, sizeof(out), NULL, sizeof(BYTES)));
    TEST_ASSERT_EQUAL_STRING("", out);
    memset(out, 'X', sizeof(out));
    TEST_ASSERT_EQUAL_INT(-1, bytes_to_lowercase_hex_0x(out, sizeof(out), NULL, sizeof(BYTES)));
    TEST_ASSERT_EQUAL_STRING("", out);

    // A NULL input of length 0 is an empty input
    TEST_ASSERT_EQUAL_INT(0, bytes_to_hex(out, sizeof(out), NULL, 0));
    TEST_ASSERT_EQUAL_STRING("", out);
    TEST_ASSERT_EQUAL_INT(0, bytes_to_hex_0x(out, sizeof(out), NULL, 0));
    TEST_ASSERT_EQUAL_STRING("0x", out);
}

void setUp(void) {}
void tearDown(void) {}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_bytes_to_hex);
    RUN_TEST(test_bytes_to_hex_empty_input);
    RUN_TEST(test_bytes_to_hex_buffer_too_small);
    RUN_TEST(test_bytes_to_hex_zero_size_buffer_untouched);
    RUN_TEST(test_bytes_to_hex_huge_length_rejected);
    RUN_TEST(test_hex_string_sizes);
    RUN_TEST(test_bytes_to_hex_0x);
    RUN_TEST(test_bytes_to_hex_0x_empty_input);
    RUN_TEST(test_bytes_to_hex_0x_buffer_too_small);
    RUN_TEST(test_bytes_to_hex_0x_zero_size_buffer_untouched);
    RUN_TEST(test_bytes_to_hex_null_pointers);
    return UNITY_END();
}
