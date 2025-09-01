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
// When defined cmocka redefine malloc/free which does not work well with
// address-sanitizer
#undef UNIT_TESTING
#include <cmocka.h>
#define UNIT_TESTING
#else
#include <cmocka.h>
#endif

#include "app_storage.h"
#include "app_storage_internal.h"
#include "app_storage_stubs.h"
#include "os_nvm.h"

/* Defines */
#define INITIAL_SIZE      20
#define ADDITIONALL_SIZE  33
#define ADDITIONALL_SIZE2 540
#define ADDITIONALL_SIZE3 1033
_Static_assert(INITIAL_SIZE < ADDITIONALL_SIZE, "Test condition is broken");
_Static_assert(ADDITIONALL_SIZE < ADDITIONALL_SIZE2, "Test condition is broken");
_Static_assert(ADDITIONALL_SIZE2 < ADDITIONALL_SIZE3, "Test condition is broken");

#define DATA_SIZE   200
#define SLOT_NUMBER 20

/* Typedefs */
typedef struct slot_s {
    uint32_t key;
    uint8_t  data[DATA_SIZE];
} slot_t;

typedef struct app_storage_data_s {
    uint32_t version;
    uint8_t  initialized;
    uint32_t slot_number;
    slot_t   slot[SLOT_NUMBER];
} app_storage_data_t;
_Static_assert(sizeof(app_storage_data_t) <= APP_STORAGE_SIZE,
               "The application storage size requested in Makefile is not sufficient");

/**
 * App storage accessors
 */
#define APP_STORAGE_WRITE_ALL(src_buf) app_storage_write(src_buf, sizeof(app_storage_data_t), 0)

#define APP_STORAGE_WRITE_F(field, src_buf) \
    app_storage_write(                      \
        src_buf, sizeof(((app_storage_data_t *) 0)->field), offsetof(app_storage_data_t, field))

#define APP_STORAGE_READ_ALL(dst_buf) app_storage_read(dst_buf, sizeof(app_storage_data_t), 0)

#define APP_STORAGE_READ_F(field, dst_buf) \
    app_storage_read(                      \
        dst_buf, sizeof(((app_storage_data_t *) 0)->field), offsetof(app_storage_data_t, field))

// app_storage.h private
extern app_storage_t app_storage_real;
bool                 app_storage_is_initalized(void);

/* Local prototypes */
static void test_write_read_from_empty(void **state __attribute__((unused)));
static void test_app_style_from_empty(void **state __attribute__((unused)));

/* Functions */
static int setup_from_empty(void **state)
{
    app_storage_init();
    return 0;
}

static int teardown(void **state)
{
    zero_out_storage();
    return 0;
}

static int setup_from_prepared(void **state)
{
    /* Prepare storage */
    app_storage_init();
    test_write_read_from_empty(state);

    /* Reinit storage */
    app_storage_init();

    return 0;
}

static int setup_from_prepared_app_style(void **state)
{
    /* Prepare storage */
    app_storage_init();
    test_app_style_from_empty(state);

    /* Reinit storage */
    app_storage_init();
    return 0;
}

/* Basic getter functions with initially empty app storage */
static void test_getters_from_empty(void **state __attribute__((unused)))
{
    /* app_storage_get_size() return 0 applicative data size with the empty storage */
    assert_int_equal(app_storage_get_size(), 0);
    /* The properties are fixed in CMakeListList.txt as APP_STORAGE_PROP_SETTINGS |
     * APP_STORAGE_PROP_DATA */
    assert_int_equal(app_storage_get_properties(),
                     APP_STORAGE_PROP_SETTINGS | APP_STORAGE_PROP_DATA);
}

/* Test that corruption from empty storage is detected */
static void test_corrupted_storage_from_empty(void **state __attribute__((unused)))
{
    assert_true(app_storage_is_initalized());
    // --- Simulate corrupted header
    app_storage_header_t header = app_storage_real.header;
    header.data_version += 1;
    // Change header with no CRC update
    nvm_write((void *) &app_storage_real.header, &header, sizeof(header));
    // Ensure invalid CRC
    assert_false(app_storage_is_initalized());

    // --- Simulate corrupted data
    setup_from_empty(NULL);
    assert_true(app_storage_is_initalized());
    uint8_t buf[20] = {0};
    memset(buf, 0xAA, sizeof(buf));
    assert_int_equal(app_storage_write(buf, sizeof(buf), 0), sizeof(buf));
    // Change data with no CRC update
    buf[sizeof(buf) - 1] = 0xAB;
    nvm_write((void *) &app_storage_real.data, buf, sizeof(buf));
    // Ensure invalid CRC
    assert_false(app_storage_is_initalized());
}

/* Test that corruption from prepared storage is detected */
static void test_corrupted_storage_from_prepared(void **state __attribute__((unused)))
{
    assert_true(app_storage_is_initalized());
    // --- Simulate corrupted header
    app_storage_header_t header = app_storage_real.header;
    header.data_version += 1;
    // Change header with no CRC update
    nvm_write((void *) &app_storage_real.header, &header, sizeof(header));
    // Ensure invalid CRC
    assert_false(app_storage_is_initalized());

    // --- Simulate corrupted data
    setup_from_prepared(NULL);
    assert_true(app_storage_is_initalized());
    uint8_t data[INITIAL_SIZE + ADDITIONALL_SIZE] = {0};
    app_storage_read(data, INITIAL_SIZE + ADDITIONALL_SIZE, 0);
    // Change data with no CRC update
    data[INITIAL_SIZE + ADDITIONALL_SIZE - 1]++;
    nvm_write((void *) &app_storage_real.data, data, INITIAL_SIZE + ADDITIONALL_SIZE);
    // Ensure invalid CRC
    assert_false(app_storage_is_initalized());
}

/* Read error cases with initially empty storage */
static void test_read_error_from_empty(void **state __attribute__((unused)))
{
    /* buf = NULL */
    assert_int_equal(app_storage_read(NULL, 5, 0), APP_STORAGE_ERR_INVALID_ARGUMENT);

    /* nbytes = 0 */
    uint8_t buf[20];
    memset(buf, 0xAA, sizeof(buf));
    assert_int_equal(app_storage_read(buf, 0, 0), APP_STORAGE_ERR_INVALID_ARGUMENT);

    /* size + offset integer overflow */
    assert_int_equal(app_storage_read(buf, UINT32_MAX - 4, 5), APP_STORAGE_ERR_INVALID_ARGUMENT);

    /* Reading 1 byte with 0 offset on empty app storage */
    assert_int_equal(app_storage_read(buf, 1, 0), APP_STORAGE_ERR_NO_DATA_AVAILABLE);
}

/* Write error cases with initially empty storage */
static void test_write_error_from_empty(void **state __attribute__((unused)))
{
    /* buf = NULL */
    assert_int_equal(app_storage_write(NULL, 5, 0), APP_STORAGE_ERR_INVALID_ARGUMENT);

    /* nbytes = 0 */
    uint8_t buf[20];
    memset(buf, 0xAA, sizeof(buf));
    assert_int_equal(app_storage_write(buf, 0, 0), APP_STORAGE_ERR_INVALID_ARGUMENT);

    /* size + offset integer overflow */
    assert_int_equal(app_storage_write(buf, UINT32_MAX - 6, 7), APP_STORAGE_ERR_INVALID_ARGUMENT);

    /* Wring outside APP_STORAGE_SIZE */
    assert_int_equal(app_storage_write(buf, APP_STORAGE_SIZE + 1, 0), APP_STORAGE_ERR_OVERFLOW);

    /* Wring outside APP_STORAGE_SIZE */
    assert_int_equal(app_storage_write(buf, 1, APP_STORAGE_SIZE), APP_STORAGE_ERR_OVERFLOW);
}

/* data_version combinations with initially empty storage */
static void test_data_version_from_empty(void **state __attribute__((unused)))
{
    /* The initial applicative data version is 1 */
    assert_int_equal(app_storage_get_data_version(), APP_STORAGE_INITIAL_APP_DATA_VERSION);

    /* The data version is manually incremented or set - not linked to writes */
    uint8_t buf_in[INITIAL_SIZE + ADDITIONALL_SIZE];
    for (uint32_t i = 0; i < sizeof(buf_in); i++) {
        buf_in[i] = i + 1;
    }
    assert_int_equal(app_storage_write(buf_in, INITIAL_SIZE + ADDITIONALL_SIZE, 0),
                     INITIAL_SIZE + ADDITIONALL_SIZE);
    assert_int_equal(app_storage_get_data_version(), APP_STORAGE_INITIAL_APP_DATA_VERSION);

    /* Increment */
    app_storage_increment_data_version();
    assert_int_equal(app_storage_get_data_version(), APP_STORAGE_INITIAL_APP_DATA_VERSION + 1);
    app_storage_increment_data_version();
    assert_int_equal(app_storage_get_data_version(), APP_STORAGE_INITIAL_APP_DATA_VERSION + 2);

    /* Set */
    const uint32_t ver = 0xA5A53256;
    app_storage_set_data_version(ver);
    assert_int_equal(app_storage_get_data_version(), ver);
    app_storage_increment_data_version();
    assert_int_equal(app_storage_get_data_version(), ver + 1);

    /* Set maximum */
    app_storage_set_data_version(UINT32_MAX);
    assert_int_equal(app_storage_get_data_version(), UINT32_MAX);
    app_storage_increment_data_version();
    assert_int_equal(app_storage_get_data_version(), APP_STORAGE_INITIAL_APP_DATA_VERSION);
}

/* Write/read/get_size combinations with initially empty storage */
static void test_write_read_from_empty(void **state __attribute__((unused)))
{
    /* Checking data version */
    assert_int_equal(app_storage_get_data_version(), APP_STORAGE_INITIAL_APP_DATA_VERSION);

    uint8_t buf_in[INITIAL_SIZE + ADDITIONALL_SIZE];
    for (uint32_t i = 0; i < sizeof(buf_in); i++) {
        buf_in[i] = i + 1;
    }

    /* Normal write with 0 offset */
    /* 1, 2, 3 ... */
    assert_int_equal(app_storage_write(buf_in, INITIAL_SIZE, 0), INITIAL_SIZE);
    app_storage_increment_data_version();

    /* Checking app storage size */
    assert_int_equal(app_storage_get_size(), INITIAL_SIZE);

    /* Reading the same bytes - normal case */
    uint8_t buf_out[INITIAL_SIZE + ADDITIONALL_SIZE] = {0};
    assert_int_equal(app_storage_read(buf_out, INITIAL_SIZE, 0), INITIAL_SIZE);
    assert_memory_equal(buf_in, buf_out, INITIAL_SIZE);

    /* Trying to read 1 byte more */
    assert_int_equal(app_storage_read(buf_out, INITIAL_SIZE + 1, 0),
                     APP_STORAGE_ERR_NO_DATA_AVAILABLE);

    /* Trying to read correct size but with offset = 1 */
    assert_int_equal(app_storage_read(buf_out, INITIAL_SIZE, 1), APP_STORAGE_ERR_NO_DATA_AVAILABLE);

    /* Reading with offset = 1 */
    memset(buf_out, 0, sizeof(buf_out));
    assert_int_equal(app_storage_read(buf_out, INITIAL_SIZE - 1, 1), INITIAL_SIZE - 1);
    assert_memory_equal(&buf_in[1], buf_out, INITIAL_SIZE - 1);

    /* Next write */
    /* 1, 2, 3 ... 21, 22, 23 */
    assert_int_equal(app_storage_write(&buf_in[INITIAL_SIZE], ADDITIONALL_SIZE, INITIAL_SIZE),
                     ADDITIONALL_SIZE);
    app_storage_increment_data_version();

    /* Checking app storage size */
    assert_int_equal(app_storage_get_size(), INITIAL_SIZE + ADDITIONALL_SIZE);

    /* Reading all */
    memset(buf_out, 0, sizeof(buf_out));
    assert_int_equal(app_storage_read(buf_out, INITIAL_SIZE + ADDITIONALL_SIZE, 0),
                     INITIAL_SIZE + ADDITIONALL_SIZE);
    assert_memory_equal(buf_in, &buf_out, INITIAL_SIZE + ADDITIONALL_SIZE);

    /* Reading at offset = INITIAL_SIZE */
    memset(buf_out, 0, sizeof(buf_out));
    assert_int_equal(app_storage_read(&buf_out[INITIAL_SIZE], ADDITIONALL_SIZE, INITIAL_SIZE),
                     ADDITIONALL_SIZE);
    assert_memory_equal(&buf_in[INITIAL_SIZE], &buf_out[INITIAL_SIZE], ADDITIONALL_SIZE);

    /* Rewriting in the middle and checking */
    /* 1, 2, 3 ... 20 + 0xA5, 21 + 0xA5, 22 + 0xA5 */
    for (uint32_t i = 0; i < sizeof(buf_in); i++) {
        buf_in[i] = i + 0xA5;
    }
    assert_int_equal(app_storage_write(buf_in, INITIAL_SIZE, 0), INITIAL_SIZE);
    app_storage_increment_data_version();

    /* Checking app storage size */
    assert_int_equal(app_storage_get_size(), INITIAL_SIZE + ADDITIONALL_SIZE);

    /* Reading the same bytes - normal case */
    memset(buf_out, 0, sizeof(buf_out));
    assert_int_equal(app_storage_read(buf_out, INITIAL_SIZE, 0), INITIAL_SIZE);
    assert_memory_equal(buf_in, buf_out, INITIAL_SIZE);

    /* Checking data version */
    assert_int_equal(app_storage_get_data_version(), APP_STORAGE_INITIAL_APP_DATA_VERSION + 3);
}

static void test_write_big_reset_from_empty(void **state __attribute__((unused)))
{
    /* Checking data version */
    assert_int_equal(app_storage_get_data_version(), APP_STORAGE_INITIAL_APP_DATA_VERSION);

    /* Checking data size */
    assert_int_equal(app_storage_get_size(), 0);
    uint32_t i   = 0;
    uint8_t  buf = 0;
    for (; i < APP_STORAGE_SIZE; i++) {
        buf = (uint8_t) i;
        assert_int_equal(app_storage_write(&buf, 1, i), 1);
        app_storage_increment_data_version();
        assert_int_equal(app_storage_get_data_version(),
                         APP_STORAGE_INITIAL_APP_DATA_VERSION + i + 1);
        assert_int_equal(app_storage_get_size(), i + 1);
    }

    /* Cannot write more */
    buf = (uint8_t) i;
    assert_int_equal(app_storage_write(&buf, 1, i), APP_STORAGE_ERR_OVERFLOW);
    assert_int_equal(app_storage_get_data_version(), APP_STORAGE_INITIAL_APP_DATA_VERSION + i);
    assert_int_equal(app_storage_get_size(), i);

    /* Read */
    i   = 0;
    buf = 0;
    for (; i < APP_STORAGE_SIZE; i++) {
        assert_int_equal(app_storage_read(&buf, 1, i), 1);
        assert_int_equal(buf, (uint8_t) i);
    }

    /* Reset */
    app_storage_reset();
    /* Checking data version */
    assert_int_equal(app_storage_get_data_version(), APP_STORAGE_INITIAL_APP_DATA_VERSION);
    /* Checking data size */
    assert_int_equal(app_storage_get_size(), 0);
    /* Checking properties */
    assert_int_equal(app_storage_get_properties(),
                     APP_STORAGE_PROP_SETTINGS | APP_STORAGE_PROP_DATA);

    /* Read is not possible */
    buf = 0;
    assert_int_equal(app_storage_read(&buf, 1, 0), APP_STORAGE_ERR_NO_DATA_AVAILABLE);

    /* Tricky way to read all storage */
    buf = 0;
    assert_int_equal(app_storage_write(&buf, 1, APP_STORAGE_SIZE - 1), 1);
    assert_int_equal(app_storage_get_size(), APP_STORAGE_SIZE);

    /* All zeroes at read */
    i   = 0;
    buf = 0;
    for (; i < APP_STORAGE_SIZE; i++) {
        assert_int_equal(app_storage_read(&buf, 1, i), 1);
        assert_int_equal(buf, 0);
    }
}

static void test_write_read_from_prepared(void **state __attribute__((unused)))
{
    /* Checking data version */
    assert_int_equal(app_storage_get_data_version(), APP_STORAGE_INITIAL_APP_DATA_VERSION + 3);

    /* Read and verify */
    uint8_t buf_out[INITIAL_SIZE + ADDITIONALL_SIZE2] = {0};
    uint8_t buf_in[INITIAL_SIZE + ADDITIONALL_SIZE2];
    for (uint32_t i = 0; i < INITIAL_SIZE; i++) {
        buf_in[i] = i + 0xA5;
    }
    for (uint32_t i = INITIAL_SIZE; i < INITIAL_SIZE + ADDITIONALL_SIZE; i++) {
        buf_in[i] = i + 1;
    }

    /* [1, 2, 3 ... 19] [20 + 0xA5, 21 + 0xA5, 22 + 0xA5 ... 52 + 0xA5 */
    assert_int_equal(app_storage_read(buf_out, INITIAL_SIZE + ADDITIONALL_SIZE, 0),
                     INITIAL_SIZE + ADDITIONALL_SIZE);
    assert_memory_equal(buf_in, buf_out, INITIAL_SIZE + ADDITIONALL_SIZE);

    /* Write overpassing and overplaping, read and compare */
    /* [1, 2, 3 ... 19] [20 + 0x33, 5 + 0x33, 6 + 0x33, 7 + 0x33, 8 + 0x33 ... 559 + 0x33]*/
    for (uint32_t i = INITIAL_SIZE; i < INITIAL_SIZE + ADDITIONALL_SIZE2; i++) {
        buf_in[i] = i + 0x33;
    }
    assert_int_equal(app_storage_write(&buf_in[INITIAL_SIZE], ADDITIONALL_SIZE2, INITIAL_SIZE),
                     ADDITIONALL_SIZE2);
    app_storage_increment_data_version();

    assert_int_equal(app_storage_get_size(), INITIAL_SIZE + ADDITIONALL_SIZE2);

    memset(buf_out, 0, INITIAL_SIZE + ADDITIONALL_SIZE2);
    assert_int_equal(app_storage_read(buf_out, INITIAL_SIZE + ADDITIONALL_SIZE2, 0),
                     INITIAL_SIZE + ADDITIONALL_SIZE2);
    assert_memory_equal(buf_in, buf_out, INITIAL_SIZE + ADDITIONALL_SIZE2);

    /* Checking data version */
    assert_int_equal(app_storage_get_data_version(), APP_STORAGE_INITIAL_APP_DATA_VERSION + 4);

    /* Writing at big offset */
    uint8_t buf_in3[INITIAL_SIZE + ADDITIONALL_SIZE3] = {0};
    memcpy(buf_in3, buf_in, INITIAL_SIZE + ADDITIONALL_SIZE2);
    buf_in3[INITIAL_SIZE + ADDITIONALL_SIZE3 - 1] = 0xB6;

    assert_int_equal(app_storage_write(&buf_in3[INITIAL_SIZE + ADDITIONALL_SIZE3 - 1],
                                       1,
                                       INITIAL_SIZE + ADDITIONALL_SIZE3 - 1),
                     1);
    app_storage_increment_data_version();
    assert_int_equal(app_storage_get_size(), INITIAL_SIZE + ADDITIONALL_SIZE3);

    uint8_t buf_out3[INITIAL_SIZE + ADDITIONALL_SIZE3] = {0};

    assert_int_equal(app_storage_read(buf_out3, INITIAL_SIZE + ADDITIONALL_SIZE3, 0),
                     INITIAL_SIZE + ADDITIONALL_SIZE3);
    assert_memory_equal(buf_in3, buf_out3, INITIAL_SIZE + ADDITIONALL_SIZE3);

    /* Checking data version */
    assert_int_equal(app_storage_get_data_version(), APP_STORAGE_INITIAL_APP_DATA_VERSION + 5);
}

static void test_app_style_from_empty(void **state __attribute__((unused)))
{
    /* Checking data version */
    assert_int_equal(app_storage_get_data_version(), APP_STORAGE_INITIAL_APP_DATA_VERSION);
    /* Checking data size */
    assert_int_equal(app_storage_get_size(), 0);

    uint32_t version     = 0x01;
    uint8_t  initialized = 1;
    assert_int_equal(APP_STORAGE_WRITE_F(version, &version), sizeof(version));
    assert_int_equal(APP_STORAGE_WRITE_F(initialized, &initialized), sizeof(initialized));
    app_storage_increment_data_version();

    for (uint32_t i = 0; i < SLOT_NUMBER; i++) {
        slot_t slot = {
            0xA454B0C5, {(i + 1) * 1, (i + 1) * 2, (i + 1) * 3}
        };
        assert_int_equal(APP_STORAGE_WRITE_F(slot[i], &slot), sizeof(slot));
        app_storage_increment_data_version();
    }

    uint32_t slot_number = SLOT_NUMBER;
    assert_int_equal(APP_STORAGE_WRITE_F(slot_number, &slot_number), sizeof(slot_number));
}

static void test_app_style_from_prepared(void **state __attribute__((unused)))
{
    /* Checking data version */
    assert_int_equal(app_storage_get_data_version(),
                     APP_STORAGE_INITIAL_APP_DATA_VERSION + 1 + SLOT_NUMBER);
    /* Checking data size */
    assert_int_equal(app_storage_get_size(), sizeof(app_storage_data_t));

    uint32_t version     = 0;
    uint8_t  initialized = 0;
    uint32_t slot_number = 0;
    assert_int_equal(APP_STORAGE_READ_F(version, &version), sizeof(version));
    assert_int_equal(version, 0x01);
    assert_int_equal(APP_STORAGE_READ_F(initialized, &initialized), sizeof(initialized));
    assert_int_equal(initialized, 1);
    assert_int_equal(APP_STORAGE_READ_F(slot_number, &slot_number), sizeof(slot_number));
    assert_int_equal(slot_number, SLOT_NUMBER);

    for (uint32_t i = 0; i < SLOT_NUMBER; i++) {
        const slot_t slot_c = {
            0xA454B0C5, {(i + 1) * 1, (i + 1) * 2, (i + 1) * 3}
        };
        slot_t slot = {0};
        assert_int_equal(APP_STORAGE_READ_F(slot[i], &slot), sizeof(slot));
        assert_memory_equal(&slot_c, &slot, sizeof(slot));
    }
}

int main(int argc, char **argv)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_getters_from_empty, setup_from_empty, teardown),
        cmocka_unit_test_setup_teardown(
            test_corrupted_storage_from_empty, setup_from_empty, teardown),
        cmocka_unit_test_setup_teardown(test_read_error_from_empty, setup_from_empty, teardown),
        cmocka_unit_test_setup_teardown(test_write_error_from_empty, setup_from_empty, teardown),
        cmocka_unit_test_setup_teardown(test_data_version_from_empty, setup_from_empty, teardown),
        cmocka_unit_test_setup_teardown(test_write_read_from_empty, setup_from_empty, teardown),
        cmocka_unit_test_setup_teardown(
            test_write_big_reset_from_empty, setup_from_empty, teardown),
        cmocka_unit_test_setup_teardown(
            test_write_read_from_prepared, setup_from_prepared, teardown),
        cmocka_unit_test_setup_teardown(
            test_corrupted_storage_from_prepared, setup_from_prepared, teardown),
        cmocka_unit_test_setup_teardown(test_app_style_from_empty, setup_from_empty, teardown),
        cmocka_unit_test_setup_teardown(
            test_app_style_from_prepared, setup_from_prepared_app_style, teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
