/*****************************************************************************
 *   (c) 2025 Ledger SAS.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
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

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#include "os_nvm.h"
#include "os_utils.h"

#include "tlv_library.h"

/* Helper macros */
#define TLV_HEADER_SIZE 3
#define MAX_TLV_SIZE    50

/* Helper functions */
static tlv_data_t create_tlv_data(uint8_t *buffer, uint8_t tag, size_t value_size, uint64_t value)
{
    buffer[0] = tag;
    U2BE_ENCODE(buffer, 1, value_size);

    for (size_t i = 0; i < value_size && i < 8; i++) {
        buffer[TLV_HEADER_SIZE + i] = (uint8_t) (value >> (8 * (value_size - i - 1)));
    }

    tlv_data_t tlv_data = {
        .tag   = tag,
        .value = {.ptr = buffer + TLV_HEADER_SIZE, .size = value_size,                   .offset = 0},
        .raw   = {.ptr = buffer,                   .size = TLV_HEADER_SIZE + value_size, .offset = 0}
    };

    return tlv_data;
}

static void test_null_pointer_checks(const tlv_data_t *tlv_data)
{
    uint64_t u64;
    uint32_t u32;
    uint16_t u16;
    uint8_t  u8;
    bool     b;

    assert_false(get_uint64_t_from_tlv_data(tlv_data, NULL));
    assert_false(get_uint64_t_from_tlv_data(NULL, &u64));
    assert_false(get_uint64_t_from_tlv_data(NULL, NULL));

    assert_false(get_uint32_t_from_tlv_data(tlv_data, NULL));
    assert_false(get_uint32_t_from_tlv_data(NULL, &u32));
    assert_false(get_uint32_t_from_tlv_data(NULL, NULL));

    assert_false(get_uint16_t_from_tlv_data(tlv_data, NULL));
    assert_false(get_uint16_t_from_tlv_data(NULL, &u16));
    assert_false(get_uint16_t_from_tlv_data(NULL, NULL));

    assert_false(get_uint8_t_from_tlv_data(tlv_data, NULL));
    assert_false(get_uint8_t_from_tlv_data(NULL, &u8));
    assert_false(get_uint8_t_from_tlv_data(NULL, NULL));

    assert_false(get_bool_from_tlv_data(tlv_data, NULL));
    assert_false(get_bool_from_tlv_data(NULL, &b));
    assert_false(get_bool_from_tlv_data(NULL, NULL));
}

/* Test cases */
static void test_unsigned_tlv_integer_readers(void **state)
{
    (void) state;

    typedef struct {
        uint64_t expected;
        size_t   input_size;
        bool     expect_success_u64;
        bool     expect_success_u32;
        bool     expect_success_u16;
        bool     expect_success_u8;
        bool     expect_success_bool;
    } test_vector_t;

    const test_vector_t vectors[] = {
        {0x00,               1, true,  true,  true,  true,  true },
        {0x01,               1, true,  true,  true,  true,  true },
        {0x01,               8, true,  true,  true,  true,  true },
        {0xFF,               1, true,  true,  true,  true,  false},
        {0xFFDE,             2, true,  true,  true,  false, false},
        {0xDE,               8, true,  true,  true,  true,  false},
        {0x1234,             2, true,  true,  true,  false, false},
        {0xFFFFFFFF,         4, true,  true,  false, false, false},
        {0xFFFFFFFFFF,       5, true,  false, false, false, false},
        {0x1122334455667788, 8, true,  false, false, false, false},
        {0,                  0, false, false, false, false, false},
        {0x01,               9, false, false, false, false, false},
    };

    for (size_t i = 0; i < sizeof(vectors) / sizeof(vectors[0]); i++) {
        uint8_t    buffer[TLV_HEADER_SIZE + MAX_TLV_SIZE] = {0};
        tlv_data_t tlv_data
            = create_tlv_data(buffer, 0xAB, vectors[i].input_size, vectors[i].expected);

        uint64_t mask = (vectors[i].input_size >= 8) ? UINT64_MAX
                                                     : ((1ULL << (8 * vectors[i].input_size)) - 1);

        // Test uint64_t
        uint64_t value_64;
        assert_int_equal(get_uint64_t_from_tlv_data(&tlv_data, &value_64),
                         vectors[i].expect_success_u64);
        if (vectors[i].expect_success_u64) {
            assert_int_equal(value_64, vectors[i].expected & mask);
        }

        // Test uint32_t
        uint32_t value_32;
        assert_int_equal(get_uint32_t_from_tlv_data(&tlv_data, &value_32),
                         vectors[i].expect_success_u32);
        if (vectors[i].expect_success_u32) {
            assert_int_equal(value_32, (uint32_t) (vectors[i].expected & mask));
        }

        // Test uint16_t
        uint16_t value_16;
        assert_int_equal(get_uint16_t_from_tlv_data(&tlv_data, &value_16),
                         vectors[i].expect_success_u16);
        if (vectors[i].expect_success_u16) {
            assert_int_equal(value_16, (uint16_t) (vectors[i].expected & mask));
        }

        // Test uint8_t
        uint8_t value_8;
        assert_int_equal(get_uint8_t_from_tlv_data(&tlv_data, &value_8),
                         vectors[i].expect_success_u8);
        if (vectors[i].expect_success_u8) {
            assert_int_equal(value_8, (uint8_t) (vectors[i].expected & mask));
        }

        // Test bool
        bool value_bool;
        assert_int_equal(get_bool_from_tlv_data(&tlv_data, &value_bool),
                         vectors[i].expect_success_bool);
        if (vectors[i].expect_success_bool) {
            assert_int_equal(value_bool, (vectors[i].expected != 0));
        }

        test_null_pointer_checks(&tlv_data);
    }
}

static void test_unsigned_tlv_buffer_readers(void **state)
{
    (void) state;

    uint8_t    test_data[] = {0xAB, 0x00, 0x03, 0xDE, 0xAD, 0xBE};
    tlv_data_t tlv_data    = {
           .tag   = test_data[0],
           .value = {.ptr = test_data + 3, .size = 3,                 .offset = 0},
           .raw   = {.ptr = test_data,     .size = sizeof(test_data), .offset = 0}
    };
    buffer_t out;

    // Valid: exact size match
    assert_true(get_buffer_from_tlv_data(&tlv_data, &out, 3, 3));
    assert_int_equal(out.size, 3);
    assert_memory_equal(out.ptr, tlv_data.value.ptr, 3);

    // Valid: within range
    assert_true(get_buffer_from_tlv_data(&tlv_data, &out, 2, 4));
    assert_int_equal(out.size, 3);

    // Valid: no size restriction
    assert_true(get_buffer_from_tlv_data(&tlv_data, &out, 0, 0));
    assert_int_equal(out.size, 3);

    // Invalid: min_size > value.size
    assert_false(get_buffer_from_tlv_data(&tlv_data, &out, 4, 0));

    // Invalid: max_size < value.size
    assert_false(get_buffer_from_tlv_data(&tlv_data, &out, 0, 2));

    // Invalid: NULL pointers
    assert_false(get_buffer_from_tlv_data(NULL, &out, 0, 0));
    assert_false(get_buffer_from_tlv_data(&tlv_data, NULL, 0, 0));
}

#define MAKE_TLV(str, len)                                               \
    ((tlv_data_t){                                                       \
        .tag   = 0x01,                                                   \
        .value = {.ptr = (uint8_t *) (str), .size = (len), .offset = 0}, \
        .raw   = {.ptr = (uint8_t *) (str), .size = (len), .offset = 0}, \
    })

static void test_unsigned_tlv_string_readers(void **state)
{
    (void) state;

    char out[32];

    // Valid: simple string
    const char *s1   = "hello";
    tlv_data_t  tlv1 = MAKE_TLV(s1, strlen(s1));
    assert_true(get_string_from_tlv_data(&tlv1, out, 0, sizeof(out)));
    assert_string_equal(out, "hello");

    // Valid: min_length matches
    assert_true(get_string_from_tlv_data(&tlv1, out, 5, sizeof(out)));
    assert_string_equal(out, "hello");

    // Valid: no restrictions
    assert_true(get_string_from_tlv_data(&tlv1, out, 0, 0));
    assert_string_equal(out, "hello");

    // Valid: exact buffer size
    assert_true(get_string_from_tlv_data(&tlv1, out, 0, 6));
    assert_string_equal(out, "hello");

    // Valid: empty string
    tlv_data_t tlv_empty = MAKE_TLV("", 0);
    assert_true(get_string_from_tlv_data(&tlv_empty, out, 0, sizeof(out)));
    assert_string_equal(out, "");

    // Invalid: min_length too large
    assert_false(get_string_from_tlv_data(&tlv1, out, 6, sizeof(out)));

    // Invalid: buffer too small
    assert_false(get_string_from_tlv_data(&tlv1, out, 0, 5));

    // Invalid: embedded null
    char       s_null[] = {'a', 'b', '\0', 'c', 'd'};
    tlv_data_t tlv_null = MAKE_TLV(s_null, 5);
    assert_false(get_string_from_tlv_data(&tlv_null, out, 0, sizeof(out)));

    // Invalid: NULL pointers
    assert_false(get_string_from_tlv_data(NULL, out, 0, sizeof(out)));
    assert_false(get_string_from_tlv_data(&tlv1, NULL, 0, sizeof(out)));
}

static TLV_flag_t tag_to_flag_function(TLV_tag_t tag)
{
    switch (tag) {
        case 0x01:
            return 0x01;
        case 0x02:
            return 0x02;
        case 0x04:
            return 0x04;
        default:
            return 0;
    }
}

static void test_tlv_check_received_tags(void **state)
{
    (void) state;

    TLV_reception_t received = {.flags = 0x03, .tag_to_flag_function = tag_to_flag_function};

    TLV_tag_t tags_present[] = {0x01, 0x02};
    assert_true(tlv_check_received_tags(received, tags_present, 2));

    TLV_tag_t tags_missing[] = {0x01, 0x02, 0x04};
    assert_false(tlv_check_received_tags(received, tags_missing, 3));

    TLV_tag_t tags_unknown[] = {0x01, 0x99};
    assert_false(tlv_check_received_tags(received, tags_unknown, 2));

    TLV_tag_t tags_empty[] = {};
    assert_true(tlv_check_received_tags(received, tags_empty, 0));

    TLV_tag_t tags_single[] = {0x02};
    assert_true(tlv_check_received_tags(received, tags_single, 1));

    TLV_reception_t received_none = {.flags = 0x00, .tag_to_flag_function = tag_to_flag_function};
    assert_false(tlv_check_received_tags(received_none, tags_present, 2));
}

int main(int argc, char **argv)
{
    (void) argc;
    (void) argv;

    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_unsigned_tlv_integer_readers),
        cmocka_unit_test(test_unsigned_tlv_buffer_readers),
        cmocka_unit_test(test_unsigned_tlv_string_readers),
        cmocka_unit_test(test_tlv_check_received_tags),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
