#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "unity.h"

#include "parser.h"

void test_apdu_parser(void)
{
    uint8_t apdu_bad_min_len[] = {0xE0, 0x03, 0x00};              // less than 4 bytes
    uint8_t apdu_bad_lc[]      = {0xE0, 0x03, 0x00, 0x00, 0x01};  // Lc = 1 but no data
    uint8_t apdu_no_lc[]       = {0xE0, 0x03, 0x01, 0x02};
    uint8_t apdu_no_data[]     = {0xE0, 0x03, 0x01, 0x02, 0x00};
    uint8_t apdu[]             = {0xE0, 0x03, 0x01, 0x02, 0x05, 0x00, 0x01, 0x02, 0x03, 0x04};

    command_t cmd;

    memset(&cmd, 0, sizeof(cmd));
    TEST_ASSERT_FALSE(apdu_parser(&cmd, apdu_bad_min_len, sizeof(apdu_bad_min_len)));

    memset(&cmd, 0, sizeof(cmd));
    TEST_ASSERT_FALSE(apdu_parser(&cmd, apdu_bad_lc, sizeof(apdu_bad_min_len)));

    memset(&cmd, 0, sizeof(cmd));
    TEST_ASSERT_TRUE(apdu_parser(&cmd, apdu_no_lc, sizeof(apdu_no_lc)));
    TEST_ASSERT_EQUAL_INT(cmd.cla, 0xE0);
    TEST_ASSERT_EQUAL_INT(cmd.ins, 0x03);
    TEST_ASSERT_EQUAL_INT(cmd.p1, 0x01);
    TEST_ASSERT_EQUAL_INT(cmd.p2, 0x02);
    TEST_ASSERT_EQUAL_INT(cmd.lc, 0);
    TEST_ASSERT_NULL(cmd.data);

    memset(&cmd, 0, sizeof(cmd));
    TEST_ASSERT_TRUE(apdu_parser(&cmd, apdu_no_data, sizeof(apdu_no_data)));
    TEST_ASSERT_EQUAL_INT(cmd.cla, 0xE0);
    TEST_ASSERT_EQUAL_INT(cmd.ins, 0x03);
    TEST_ASSERT_EQUAL_INT(cmd.p1, 0x01);
    TEST_ASSERT_EQUAL_INT(cmd.p2, 0x02);
    TEST_ASSERT_EQUAL_INT(cmd.lc, 0);
    TEST_ASSERT_NULL(cmd.data);

    memset(&cmd, 0, sizeof(cmd));
    TEST_ASSERT_TRUE(apdu_parser(&cmd, apdu, sizeof(apdu)));
    TEST_ASSERT_EQUAL_INT(cmd.cla, 0xE0);
    TEST_ASSERT_EQUAL_INT(cmd.ins, 0x03);
    TEST_ASSERT_EQUAL_INT(cmd.p1, 0x01);
    TEST_ASSERT_EQUAL_INT(cmd.p2, 0x02);
    TEST_ASSERT_EQUAL_INT(cmd.lc, 5);
    TEST_ASSERT_NOT_NULL(cmd.data);
    TEST_ASSERT_EQUAL_MEMORY(cmd.data, ((uint8_t[]){0x00, 0x01, 0x02, 0x03, 0x04}), cmd.lc);
}

void setUp(void) {}
void tearDown(void) {}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_apdu_parser);
    return UNITY_END();
}
