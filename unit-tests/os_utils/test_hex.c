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
    return UNITY_END();
}
