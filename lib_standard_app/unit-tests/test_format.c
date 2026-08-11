#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "unity.h"

#include "format.h"

void test_format_i64(void)
{
    char temp[22] = {0};

    int64_t value = 0;
    TEST_ASSERT_TRUE(format_i64(temp, sizeof(temp), value));
    TEST_ASSERT_EQUAL_STRING(temp, "0");

    value = (int64_t) 9223372036854775807ull;  // MAX_INT64
    memset(temp, 0, sizeof(temp));
    TEST_ASSERT_TRUE(format_i64(temp, sizeof(temp), value));
    TEST_ASSERT_EQUAL_STRING(temp, "9223372036854775807");

    // buffer too small
    TEST_ASSERT_FALSE(format_i64(temp, sizeof(temp) - 5, value));

    value = (int64_t) -9223372036854775808ull;  // MIN_INT64
    memset(temp, 0, sizeof(temp));
    TEST_ASSERT_TRUE(format_i64(temp, sizeof(temp), value));
    TEST_ASSERT_EQUAL_STRING(temp, "-9223372036854775808");
}

void test_format_u64(void)
{
    char temp[21] = {0};

    uint64_t value = 0;
    TEST_ASSERT_TRUE(format_u64(temp, sizeof(temp), value));
    TEST_ASSERT_EQUAL_STRING(temp, "0");

    value = (uint64_t) 18446744073709551615ull;  // MAX_UNT64
    memset(temp, 0, sizeof(temp));
    TEST_ASSERT_TRUE(format_u64(temp, sizeof(temp), value));
    TEST_ASSERT_EQUAL_STRING(temp, "18446744073709551615");

    // buffer too small
    TEST_ASSERT_FALSE(format_u64(temp, sizeof(temp) - 5, value));
}

void test_format_fpu64(void)
{
    char temp[22] = {0};

    uint64_t amount = 100000000ull;  // satoshi
    memset(temp, 0, sizeof(temp));
    TEST_ASSERT_TRUE(format_fpu64(temp, sizeof(temp), amount, 8));
    TEST_ASSERT_EQUAL_STRING(temp, "1.00000000");  // BTC

    amount = 24964823ull;  // satoshi
    memset(temp, 0, sizeof(temp));
    TEST_ASSERT_TRUE(format_fpu64(temp, sizeof(temp), amount, 8));
    TEST_ASSERT_EQUAL_STRING(temp, "0.24964823");  // BTC

    amount = 100ull;  // satoshi
    memset(temp, 0, sizeof(temp));
    TEST_ASSERT_TRUE(format_fpu64(temp, sizeof(temp), amount, 8));
    TEST_ASSERT_EQUAL_STRING(temp, "0.00000100");  // BTC
    // buffer too small
    TEST_ASSERT_FALSE(format_fpu64(temp, sizeof(temp) - 16, amount, 8));

    char temp2[50] = {0};

    amount = 1000000000000000000ull;  // wei
    TEST_ASSERT_TRUE(format_fpu64(temp2, sizeof(temp2), amount, 18));
    TEST_ASSERT_EQUAL_STRING(temp2, "1.000000000000000000");  // ETH

    // buffer too small
    TEST_ASSERT_FALSE(format_fpu64(temp2, sizeof(temp2) - 20, amount, 18));
}

void test_format_fpu64_trimmed(void)
{
    char temp[22] = {0};

    uint64_t amount = 100000000ull;  // satoshi
    memset(temp, 0, sizeof(temp));
    TEST_ASSERT_TRUE(format_fpu64_trimmed(temp, sizeof(temp), amount, 8));
    TEST_ASSERT_EQUAL_STRING(temp, "1");  // BTC

    amount = 24964823ull;  // satoshi
    memset(temp, 0, sizeof(temp));
    TEST_ASSERT_TRUE(format_fpu64_trimmed(temp, sizeof(temp), amount, 8));
    TEST_ASSERT_EQUAL_STRING(temp, "0.24964823");  // BTC

    amount = 100ull;  // satoshi
    memset(temp, 0, sizeof(temp));
    TEST_ASSERT_TRUE(format_fpu64_trimmed(temp, sizeof(temp), amount, 8));
    TEST_ASSERT_EQUAL_STRING(temp, "0.000001");  // BTC

    amount = 1000000000ull;  // satoshi
    memset(temp, 0, sizeof(temp));
    TEST_ASSERT_TRUE(format_fpu64_trimmed(temp, sizeof(temp), amount, 8));
    TEST_ASSERT_EQUAL_STRING(temp, "10");  // BTC
}

void test_format_hex(void)
{
    uint8_t address[] = {0xde, 0xb,  0x29, 0x56, 0x69, 0xa9, 0xfd, 0x93, 0xd5, 0xf2,
                         0x8d, 0x9e, 0xc8, 0x5e, 0x40, 0xf4, 0xcb, 0x69, 0x7b, 0xae};
    char    output[2 * sizeof(address) + 1] = {0};

    TEST_ASSERT_EQUAL_INT(2 * sizeof(address) + 1,
                          format_hex(address, sizeof(address), output, sizeof(output)));
    TEST_ASSERT_EQUAL_STRING(output, "DE0B295669A9FD93D5F28D9EC85E40F4CB697BAE");
    TEST_ASSERT_EQUAL_INT(-1, format_hex(address, sizeof(address), output, sizeof(address)));
}

void setUp(void) {}
void tearDown(void) {}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_format_i64);
    RUN_TEST(test_format_u64);
    RUN_TEST(test_format_fpu64);
    RUN_TEST(test_format_fpu64_trimmed);
    RUN_TEST(test_format_hex);
    return UNITY_END();
}
