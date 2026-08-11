#include <stdint.h>
#include <stdbool.h>

#include "unity.h"

#include "bip32.h"

void test_bip32_format(void)
{
    char output[30];
    bool b = false;

    b = bip32_path_format(
        (const uint32_t[5]){0x8000002C, 0x80000000, 0x80000000, 0, 0}, 5, output, sizeof(output));
    TEST_ASSERT_TRUE(b);
    TEST_ASSERT_EQUAL_STRING(output, "44'/0'/0'/0/0");

    b = bip32_path_format(
        (const uint32_t[5]){0x8000002C, 0x80000001, 0x80000000, 0, 0}, 5, output, sizeof(output));
    TEST_ASSERT_TRUE(b);
    TEST_ASSERT_EQUAL_STRING(output, "44'/1'/0'/0/0");
}

void test_bad_bip32_format(void)
{
    char output[30];
    bool b = true;

    // More than MAX_BIP32_PATH (=10)
    b = bip32_path_format(
        (const uint32_t[11]){0x8000002C, 0x80000000, 0x80000000, 0, 0, 0, 0, 0, 0, 0, 0},
        11,
        output,
        sizeof(output));
    TEST_ASSERT_FALSE(b);

    // No BIP32 path (=0)
    b = bip32_path_format(NULL, 0, output, sizeof(output));
    TEST_ASSERT_FALSE(b);
}

void test_bip32_read(void)
{
    // clang-format off
    uint8_t input[20] = {
        0x80, 0x00, 0x00, 0x2C,
        0x80, 0x00, 0x00, 0x01,
        0x80, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00
    };
    uint32_t expected[5] = {0x8000002C, 0x80000001, 0x80000000, 0, 0};
    uint32_t output[5] = {0};
    bool b = false;

    b = bip32_path_read(input, sizeof(input), output, 5);
    TEST_ASSERT_TRUE(b);
    TEST_ASSERT_EQUAL_MEMORY(output, expected, 5);
}

void test_bad_bip32_read(void) {
    // clang-format off
    uint8_t input[20] = {
        0x80, 0x00, 0x00, 0x2C,
        0x80, 0x00, 0x00, 0x01,
        0x80, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00
    };
    uint32_t output[10] = {0};

    // buffer too small (5 BIP32 paths instead of 10)
    TEST_ASSERT_FALSE(bip32_path_read(input, sizeof(input), output, 10));

    // No BIP32 path
    TEST_ASSERT_FALSE(bip32_path_read(input, sizeof(input), output, 0));

    // More than MAX_BIP32_PATH (=10)
    TEST_ASSERT_FALSE(bip32_path_read(input, sizeof(input), output, 20));
}

void test_bip32_format_simple(void)
{
    char output[30];
    bool b = false;

    path_bip32_t bip32 = {
        .path   = {0x8000002C, 0x80000000, 0x80000000, 0, 0},
        .length = 5,
    };
    b = bip32_path_format_simple(&bip32, output, sizeof(output));
    TEST_ASSERT_TRUE(b);
    TEST_ASSERT_EQUAL_STRING(output, "44'/0'/0'/0/0");

    path_bip32_t bip32_2 = {
        .path   = {0x8000002C, 0x80000001, 0x80000000, 0, 0},
        .length = 5,
    };
    b = bip32_path_format_simple(&bip32_2, output, sizeof(output));
    TEST_ASSERT_TRUE(b);
    TEST_ASSERT_EQUAL_STRING(output, "44'/1'/0'/0/0");

    // Single-component path
    path_bip32_t bip32_single = {
        .path   = {0x80000000},
        .length = 1,
    };
    b = bip32_path_format_simple(&bip32_single, output, sizeof(output));
    TEST_ASSERT_TRUE(b);
    TEST_ASSERT_EQUAL_STRING(output, "0'");
}

void test_bad_bip32_format_simple(void)
{
    char output[30];
    bool b = true;

    path_bip32_t bip32 = {
        .path   = {0x8000002C},
        .length = 1,
    };

    // NULL bip32
    b = bip32_path_format_simple(NULL, output, sizeof(output));
    TEST_ASSERT_FALSE(b);

    // NULL output buffer
    b = bip32_path_format_simple(&bip32, NULL, sizeof(output));
    TEST_ASSERT_FALSE(b);

    // Zero-length output buffer
    b = bip32_path_format_simple(&bip32, output, 0);
    TEST_ASSERT_FALSE(b);

    // length == 0 (rejected by bip32_path_format)
    path_bip32_t bip32_zero = {.path = {0}, .length = 0};
    b = bip32_path_format_simple(&bip32_zero, output, sizeof(output));
    TEST_ASSERT_FALSE(b);

    // length > MAX_BIP32_PATH (rejected by bip32_path_format)
    path_bip32_t bip32_too_long = {.path = {0}, .length = MAX_BIP32_PATH + 1};
    b = bip32_path_format_simple(&bip32_too_long, output, sizeof(output));
    TEST_ASSERT_FALSE(b);

    // Output buffer too small to hold the formatted string
    path_bip32_t bip32_long = {
        .path   = {0x8000002C, 0x80000000, 0x80000000, 0, 0},
        .length = 5,
    };
    char tiny[5];
    b = bip32_path_format_simple(&bip32_long, tiny, sizeof(tiny));
    TEST_ASSERT_FALSE(b);
}

void setUp(void) {}
void tearDown(void) {}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_bip32_format);
    RUN_TEST(test_bad_bip32_format);
    RUN_TEST(test_bip32_read);
    RUN_TEST(test_bad_bip32_read);
    RUN_TEST(test_bip32_format_simple);
    RUN_TEST(test_bad_bip32_format_simple);
    return UNITY_END();
}
