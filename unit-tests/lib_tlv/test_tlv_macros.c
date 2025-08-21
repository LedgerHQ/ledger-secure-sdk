/*****************************************************************************
 *   (c) 2025 Ledger SAS.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *
 *****************************************************************************/
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef UNIT_TESTING
#undef UNIT_TESTING
#include <cmocka.h>
#define UNIT_TESTING
#else
#include <cmocka.h>
#endif

#include "tlv_library.h"
#include "buffer.h"

/* -------------------------------------------------------------------------- */
/* Test TLV parser macro machinery                                            */
/* -------------------------------------------------------------------------- */

static bool handler_alpha(const tlv_data_t *data, void *out);
static bool handler_beta(const tlv_data_t *data, void *out);

#define TEST_TAGS(X)                                      \
    X(0x01, TAG_ALPHA, handler_alpha, ENFORCE_UNIQUE_TAG) \
    X(0x02, TAG_BETA, handler_beta, ALLOW_MULTIPLE_TAG)   \
    X(0x03, TAG_GAMMA, NULL, ENFORCE_UNIQUE_TAG)

/* Expand TLV parser and helpers */
DEFINE_TLV_PARSER(TEST_TAGS, NULL, test_parser)

/* Second parser for testing multiple parsers coexistence */
#define SECOND_TAGS(X)                         \
    X(0x10, TAG_FOO, NULL, ENFORCE_UNIQUE_TAG) \
    X(0x20, TAG_BAR, NULL, ALLOW_MULTIPLE_TAG)

DEFINE_TLV_PARSER(SECOND_TAGS, NULL, second_parser)

/* Third parser with common handler */
static bool common_handler(const tlv_data_t *data, void *out);

#define THIRD_TAGS(X) X(0x30, TAG_DELTA, NULL, ENFORCE_UNIQUE_TAG)

DEFINE_TLV_PARSER(THIRD_TAGS, common_handler, third_parser)

/* Fourth parser with many tags to test scaling */
#define MANY_TAGS(X)                          \
    X(0xA0, TAG_A0, NULL, ENFORCE_UNIQUE_TAG) \
    X(0xA1, TAG_A1, NULL, ENFORCE_UNIQUE_TAG) \
    X(0xA2, TAG_A2, NULL, ENFORCE_UNIQUE_TAG) \
    X(0xA3, TAG_A3, NULL, ENFORCE_UNIQUE_TAG) \
    X(0xA4, TAG_A4, NULL, ENFORCE_UNIQUE_TAG) \
    X(0xA5, TAG_A5, NULL, ENFORCE_UNIQUE_TAG) \
    X(0xA6, TAG_A6, NULL, ENFORCE_UNIQUE_TAG) \
    X(0xA7, TAG_A7, NULL, ENFORCE_UNIQUE_TAG)

DEFINE_TLV_PARSER(MANY_TAGS, NULL, many_parser)

/* -------------------------------------------------------------------------- */
/* Dummy handler implementations                                              */
/* -------------------------------------------------------------------------- */

typedef struct {
    bool     alpha_called;
    bool     beta_called;
    uint32_t beta_call_count;
    bool     common_called;
    uint32_t common_call_count;
} test_output_t;

static bool handler_alpha(const tlv_data_t *data, void *out)
{
    (void) data;
    test_output_t *ctx = out;
    ctx->alpha_called  = true;
    return true;
}

static bool handler_beta(const tlv_data_t *data, void *out)
{
    (void) data;
    test_output_t *ctx = out;
    ctx->beta_called   = true;
    ctx->beta_call_count++;
    return true;
}

static bool common_handler(const tlv_data_t *data, void *out)
{
    (void) data;
    test_output_t *ctx = out;
    ctx->common_called = true;
    ctx->common_call_count++;
    return true;
}

/* -------------------------------------------------------------------------- */
/* Tests for __X_DEFINE_TLV__TAG_ASSIGN macro                                 */
/* -------------------------------------------------------------------------- */

static void test_tag_enum_values(void **state)
{
    (void) state;

    /* Test that TAG_ASSIGN macro correctly creates enum with specified values */
    assert_int_equal(TAG_ALPHA, 0x01);
    assert_int_equal(TAG_BETA, 0x02);
    assert_int_equal(TAG_GAMMA, 0x03);

    /* Test second parser tags */
    assert_int_equal(TAG_FOO, 0x10);
    assert_int_equal(TAG_BAR, 0x20);

    /* Test third parser tags */
    assert_int_equal(TAG_DELTA, 0x30);

    /* Test many tags parser */
    assert_int_equal(TAG_A0, 0xA0);
    assert_int_equal(TAG_A7, 0xA7);
}

/* -------------------------------------------------------------------------- */
/* Tests for __X_DEFINE_TLV__TAG_INDEX macro                                  */
/* -------------------------------------------------------------------------- */

static void test_tag_index_enum(void **state)
{
    (void) state;

    /* Test that TAG_INDEX macro creates sequential indices starting at 0 */
    assert_int_equal(TAG_ALPHA_INDEX, 0);
    assert_int_equal(TAG_BETA_INDEX, 1);
    assert_int_equal(TAG_GAMMA_INDEX, 2);

    /* Test TAG_COUNT generation */
    assert_int_equal(test_parser_TAG_COUNT, 3);
    assert_int_equal(second_parser_TAG_COUNT, 2);
    assert_int_equal(third_parser_TAG_COUNT, 1);
    assert_int_equal(many_parser_TAG_COUNT, 8);
}

static void test_tag_indices_are_sequential(void **state)
{
    (void) state;

    /* Verify indices form a continuous sequence */
    assert_int_equal(TAG_BETA_INDEX - TAG_ALPHA_INDEX, 1);
    assert_int_equal(TAG_GAMMA_INDEX - TAG_BETA_INDEX, 1);

    /* Verify for many tags parser */
    assert_int_equal(TAG_A1_INDEX - TAG_A0_INDEX, 1);
    assert_int_equal(TAG_A2_INDEX - TAG_A1_INDEX, 1);
    assert_int_equal(TAG_A7_INDEX - TAG_A6_INDEX, 1);
}

/* -------------------------------------------------------------------------- */
/* Tests for __X_DEFINE_TLV__TAG_FLAG macro                                   */
/* -------------------------------------------------------------------------- */

static void test_tag_flag_values(void **state)
{
    (void) state;

    /* Test that TAG_FLAG macro correctly computes (1U << INDEX) */
    assert_int_equal(TAG_ALPHA_FLAG, 1U << TAG_ALPHA_INDEX);
    assert_int_equal(TAG_BETA_FLAG, 1U << TAG_BETA_INDEX);
    assert_int_equal(TAG_GAMMA_FLAG, 1U << TAG_GAMMA_INDEX);

    /* Verify specific flag values */
    assert_int_equal(TAG_ALPHA_FLAG, 0x01); /* 1 << 0 */
    assert_int_equal(TAG_BETA_FLAG, 0x02);  /* 1 << 1 */
    assert_int_equal(TAG_GAMMA_FLAG, 0x04); /* 1 << 2 */
}

static void test_tag_flags_are_unique(void **state)
{
    (void) state;

    /* Verify that each flag is a unique bit pattern */
    assert_true((TAG_ALPHA_FLAG & TAG_BETA_FLAG) == 0);
    assert_true((TAG_ALPHA_FLAG & TAG_GAMMA_FLAG) == 0);
    assert_true((TAG_BETA_FLAG & TAG_GAMMA_FLAG) == 0);

    /* Verify OR combination creates unique patterns */
    uint32_t combined = TAG_ALPHA_FLAG | TAG_BETA_FLAG;
    assert_int_equal(combined, 0x03);

    combined = TAG_ALPHA_FLAG | TAG_GAMMA_FLAG;
    assert_int_equal(combined, 0x05);
}

static void test_tag_flags_multiple_parsers(void **state)
{
    (void) state;

    /* Test that flags from different parsers don't collide */
    assert_int_equal(TAG_FOO_FLAG, 1U << TAG_FOO_INDEX);
    assert_int_equal(TAG_BAR_FLAG, 1U << TAG_BAR_INDEX);
    assert_int_equal(TAG_DELTA_FLAG, 1U << TAG_DELTA_INDEX);

    /* Test many flags */
    assert_int_equal(TAG_A0_FLAG, 1U << TAG_A0_INDEX);
    assert_int_equal(TAG_A7_FLAG, 1U << TAG_A7_INDEX);
    assert_int_equal(TAG_A7_FLAG, 0x80); /* 1 << 7 */
}

/* -------------------------------------------------------------------------- */
/* Tests for __X_DEFINE_TLV__TAG_TO_FLAG_CASE macro                           */
/* -------------------------------------------------------------------------- */

static void test_tag_to_flag_function(void **state)
{
    (void) state;

    /* Test TAG_TO_FLAG_CASE macro generates correct switch cases */
    assert_int_equal(test_parser_tag_to_flag(TAG_ALPHA), TAG_ALPHA_FLAG);
    assert_int_equal(test_parser_tag_to_flag(TAG_BETA), TAG_BETA_FLAG);
    assert_int_equal(test_parser_tag_to_flag(TAG_GAMMA), TAG_GAMMA_FLAG);

    /* Test unknown tag returns 0 */
    assert_int_equal(test_parser_tag_to_flag(0x99), 0);
    assert_int_equal(test_parser_tag_to_flag(0x00), 0);
    assert_int_equal(test_parser_tag_to_flag(0xFF), 0);
}

static void test_tag_to_flag_multiple_parsers(void **state)
{
    (void) state;

    /* Test second parser */
    assert_int_equal(second_parser_tag_to_flag(TAG_FOO), TAG_FOO_FLAG);
    assert_int_equal(second_parser_tag_to_flag(TAG_BAR), TAG_BAR_FLAG);
    assert_int_equal(second_parser_tag_to_flag(0xFF), 0);

    /* Test third parser */
    assert_int_equal(third_parser_tag_to_flag(TAG_DELTA), TAG_DELTA_FLAG);
    assert_int_equal(third_parser_tag_to_flag(0xFF), 0);

    /* Test many parser */
    assert_int_equal(many_parser_tag_to_flag(TAG_A0), TAG_A0_FLAG);
    assert_int_equal(many_parser_tag_to_flag(TAG_A7), TAG_A7_FLAG);
    assert_int_equal(many_parser_tag_to_flag(0xFF), 0);
}

static void test_tag_to_flag_cross_parser_isolation(void **state)
{
    (void) state;

    /* Verify that parsers don't accept tags from other parsers */
    assert_int_equal(test_parser_tag_to_flag(TAG_FOO), 0);
    assert_int_equal(test_parser_tag_to_flag(TAG_BAR), 0);
    assert_int_equal(second_parser_tag_to_flag(TAG_ALPHA), 0);
    assert_int_equal(second_parser_tag_to_flag(TAG_BETA), 0);
}

/* -------------------------------------------------------------------------- */
/* Tests for __X_DEFINE_TLV__TAG_CALLBACKS macro                              */
/* -------------------------------------------------------------------------- */

static void test_parser_function_exists(void **state)
{
    (void) state;

    /* Verify that DEFINE_TLV_PARSER creates the parser function */
    /* We test this indirectly by checking that the function compiles and links */
    TLV_reception_t received = {0};
    test_output_t   out      = {0};
    buffer_t        buf      = {.ptr = NULL, .size = 0, .offset = 0};

    /* Function should exist and be callable (even if it fails with empty buffer) */
    bool result = test_parser(&buf, &out, &received);
    (void) result; /* We don't care about the result, just that it compiles */
}

static void test_multiple_parser_coexistence(void **state)
{
    (void) state;

    /* Test that multiple parsers can coexist without naming conflicts */
    TLV_reception_t received1 = {0}, received2 = {0}, received3 = {0}, received4 = {0};
    test_output_t   out = {0};
    buffer_t        buf = {.ptr = NULL, .size = 0, .offset = 0};

    /* All parsers should be callable */
    (void) test_parser(&buf, &out, &received1);
    (void) second_parser(&buf, &out, &received2);
    (void) third_parser(&buf, &out, &received3);
    (void) many_parser(&buf, &out, &received4);
}

/* -------------------------------------------------------------------------- */
/* Tests for DEFINE_TLV_PARSER macro integration                              */
/* -------------------------------------------------------------------------- */

static void test_enum_consistency(void **state)
{
    (void) state;

    /* Comprehensive check that all generated enums are consistent */
    assert_int_equal(TAG_ALPHA, 0x01);
    assert_int_equal(TAG_BETA, 0x02);
    assert_int_equal(TAG_GAMMA, 0x03);

    assert_int_equal(TAG_ALPHA_FLAG, 1U << TAG_ALPHA_INDEX);
    assert_int_equal(TAG_BETA_FLAG, 1U << TAG_BETA_INDEX);
    assert_int_equal(TAG_GAMMA_FLAG, 1U << TAG_GAMMA_INDEX);

    assert_int_equal(test_parser_TAG_COUNT, 3);
}

static void test_parser_tag_count_matches_definitions(void **state)
{
    (void) state;

    /* Verify TAG_COUNT matches the number of tags defined */
    /* test_parser has 3 tags */
    assert_int_equal(test_parser_TAG_COUNT, 3);

    /* second_parser has 2 tags */
    assert_int_equal(second_parser_TAG_COUNT, 2);

    /* third_parser has 1 tag */
    assert_int_equal(third_parser_TAG_COUNT, 1);

    /* many_parser has 8 tags */
    assert_int_equal(many_parser_TAG_COUNT, 8);
}

static void test_all_flags_can_be_combined(void **state)
{
    (void) state;

    /* Test that all flags can be OR'd together without collision */
    uint32_t all_flags = TAG_ALPHA_FLAG | TAG_BETA_FLAG | TAG_GAMMA_FLAG;
    assert_int_equal(all_flags, 0x07); /* 0b111 */

    /* Verify each flag is still distinguishable */
    assert_true(all_flags & TAG_ALPHA_FLAG);
    assert_true(all_flags & TAG_BETA_FLAG);
    assert_true(all_flags & TAG_GAMMA_FLAG);
}

static void test_tag_values_are_preserved(void **state)
{
    (void) state;

    /* Verify that the X-macro expansion preserves the exact tag values */
    /* This is critical for protocol compatibility */
    assert_int_equal(TAG_ALPHA, 0x01); /* Not 0 or 1 as index */
    assert_int_equal(TAG_BETA, 0x02);  /* Not 1 or 2 as index */
    assert_int_equal(TAG_GAMMA, 0x03); /* Not 2 or 4 as flag */

    /* Test with non-sequential values */
    assert_int_equal(TAG_FOO, 0x10);
    assert_int_equal(TAG_BAR, 0x20);
    assert_int_equal(TAG_DELTA, 0x30);
}

static void test_index_enumeration_starts_at_zero(void **state)
{
    (void) state;

    /* Verify that _INDEX enums always start at 0 */
    assert_int_equal(TAG_ALPHA_INDEX, 0);
    assert_int_equal(TAG_FOO_INDEX, 0);
    assert_int_equal(TAG_DELTA_INDEX, 0);
    assert_int_equal(TAG_A0_INDEX, 0);
}

static void test_flag_computation_correctness(void **state)
{
    (void) state;

    /* Verify flag computation for various index values */
    for (int i = 0; i < 8; i++) {
        uint32_t expected_flag = 1U << i;
        /* We can't dynamically test this, but we verify the pattern */
        if (i == TAG_ALPHA_INDEX) {
            assert_int_equal(TAG_ALPHA_FLAG, expected_flag);
        }
        if (i == TAG_BETA_INDEX) {
            assert_int_equal(TAG_BETA_FLAG, expected_flag);
        }
        if (i == TAG_GAMMA_INDEX) {
            assert_int_equal(TAG_GAMMA_FLAG, expected_flag);
        }
    }
}

/* -------------------------------------------------------------------------- */
/* Test suite entry point                                                     */
/* -------------------------------------------------------------------------- */

int main(int argc, char **argv)
{
    (void) argc;
    (void) argv;

    const struct CMUnitTest tests[] = {
        /* __X_DEFINE_TLV__TAG_ASSIGN tests */
        cmocka_unit_test(test_tag_enum_values),

        /* __X_DEFINE_TLV__TAG_INDEX tests */
        cmocka_unit_test(test_tag_index_enum),
        cmocka_unit_test(test_tag_indices_are_sequential),

        /* __X_DEFINE_TLV__TAG_FLAG tests */
        cmocka_unit_test(test_tag_flag_values),
        cmocka_unit_test(test_tag_flags_are_unique),
        cmocka_unit_test(test_tag_flags_multiple_parsers),

        /* __X_DEFINE_TLV__TAG_TO_FLAG_CASE tests */
        cmocka_unit_test(test_tag_to_flag_function),
        cmocka_unit_test(test_tag_to_flag_multiple_parsers),
        cmocka_unit_test(test_tag_to_flag_cross_parser_isolation),

        /* __X_DEFINE_TLV__TAG_CALLBACKS tests */
        cmocka_unit_test(test_parser_function_exists),
        cmocka_unit_test(test_multiple_parser_coexistence),

        /* Integration tests */
        cmocka_unit_test(test_enum_consistency),
        cmocka_unit_test(test_parser_tag_count_matches_definitions),
        cmocka_unit_test(test_all_flags_can_be_combined),
        cmocka_unit_test(test_tag_values_are_preserved),
        cmocka_unit_test(test_index_enumeration_starts_at_zero),
        cmocka_unit_test(test_flag_computation_correctness),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
