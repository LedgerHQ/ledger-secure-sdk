#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "unity.h"

#include "base58.h"

void test_base58(void)
{
    const char in[]           = "USm3fpXnKG5EUBx2ndxBDMPVciP5hGey2Jh4NDv6gmeo1LkMeiKrLJUUBk6Z";
    const char expected_out[] = "The quick brown fox jumps over the lazy dog.";
    uint8_t    out[100]       = {0};
    int        out_len        = base58_decode(in, sizeof(in) - 1, out, sizeof(out));
    TEST_ASSERT_EQUAL_INT(out_len, strlen(expected_out));
    TEST_ASSERT_EQUAL_STRING((char *) out, expected_out);

    const char in2[]           = "The quick brown fox jumps over the lazy dog.";
    const char expected_out2[] = "USm3fpXnKG5EUBx2ndxBDMPVciP5hGey2Jh4NDv6gmeo1LkMeiKrLJUUBk6Z";
    char       out2[100]       = {0};
    int        out_len2 = base58_encode((uint8_t *) in2, sizeof(in2) - 1, out2, sizeof(out2));
    TEST_ASSERT_EQUAL_INT(out_len2, strlen(expected_out2));
    TEST_ASSERT_EQUAL_STRING((char *) out2, expected_out2);
}

void setUp(void) {}
void tearDown(void) {}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_base58);
    return UNITY_END();
}
