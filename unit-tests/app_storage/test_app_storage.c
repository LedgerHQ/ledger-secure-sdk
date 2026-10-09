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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "unity.h"

#include "Mockos_nvm.h"
#include "Mocklcx_crc.h"
#include "app_storage.h"
#include "app_storage_internal.h"

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

/* Local prototypes */
void test_write_read_from_empty(void);
void test_app_style_from_empty(void);

/* Overrides the weak hook: records the status app_storage_init() passes */
static unsigned int callback_calls;
static int32_t      callback_status;

void app_storage_callback(int32_t status)
{
    callback_calls++;
    callback_status = status;
}

/* cx_crc32 CMock stub: data-sensitive checksum sufficient for corruption detection */
static uint32_t cx_crc32_stub(const void *buf, size_t len, int num_calls)
{
    (void) num_calls;
    uint32_t       sum = 0;
    const uint8_t *b   = buf;
    while (len--) {
        sum += *b++;
    }
    return sum;
}

/* nvm_write CMock stub: simulates NVM persistence using the in-RAM app_storage_real */
static void nvm_write_stub(void *dst_addr, void *src_addr, unsigned int src_len, int num_calls)
{
    (void) num_calls;
    uint8_t     *as_blob   = (uint8_t *) &app_storage_real;
    const size_t blob_size = sizeof(app_storage_real);
    if ((const uint8_t *) dst_addr < as_blob
        || (const uint8_t *) dst_addr + src_len > as_blob + blob_size) {
        fprintf(stderr, "App NVRAM write attempt out of boundaries\n");
        return;
    }
    memcpy(as_blob + ((const uint8_t *) dst_addr - as_blob), src_addr, src_len);
}

/* Setup / teardown helpers */
static void setup_from_empty(void)
{
    memset(&app_storage_real, 0, sizeof(app_storage_real));
    TEST_ASSERT_EQUAL_INT(APP_STORAGE_ERR_INVALID_HEADER, app_storage_init());
}

static void setup_from_prepared(void)
{
    /* Prepare storage */
    test_write_read_from_empty();

    /* Reinit storage */
    TEST_ASSERT_EQUAL_INT(APP_STORAGE_SUCCESS, app_storage_init());
}

static void setup_from_prepared_app_style(void)
{
    /* Prepare storage */
    test_app_style_from_empty();

    /* Reinit storage */
    TEST_ASSERT_EQUAL_INT(APP_STORAGE_SUCCESS, app_storage_init());
}

void setUp(void)
{
    Mockos_nvm_Init();
    nvm_write_Stub(nvm_write_stub);
    Mocklcx_crc_Init();
    cx_crc32_Stub(cx_crc32_stub);
}

void tearDown(void)
{
    Mockos_nvm_Verify();
    Mockos_nvm_Destroy();
    Mocklcx_crc_Verify();
    Mocklcx_crc_Destroy();
    memset(&app_storage_real, 0, sizeof(app_storage_real));
}

/* Basic getter functions with initially empty app storage */
void test_getters_from_empty(void)
{
    setup_from_empty();
    /* app_storage_get_size() return 0 applicative data size with the empty storage */
    TEST_ASSERT_EQUAL_INT(0, app_storage_get_size());
    /* The properties are fixed in CMakeListList.txt as APP_STORAGE_PROP_SETTINGS |
     * APP_STORAGE_PROP_DATA */
    TEST_ASSERT_EQUAL_INT(APP_STORAGE_PROP_SETTINGS | APP_STORAGE_PROP_DATA,
                          app_storage_get_properties());
}

/* Test that corruption from empty storage is detected */
void test_corrupted_storage_from_empty(void)
{
    setup_from_empty();
    // --- Simulate corrupted header
    app_storage_header_t header = app_storage_real.header;
    header.data_version += 1;
    // Change header with no CRC update
    nvm_write((void *) &app_storage_real.header, &header, sizeof(header));
    // Ensure invalid CRC
    TEST_ASSERT_EQUAL_INT(APP_STORAGE_ERR_CORRUPTED, app_storage_init());

    // --- Simulate corrupted data
    setup_from_empty();
    uint8_t buf[20] = {0};
    memset(buf, 0xAA, sizeof(buf));
    TEST_ASSERT_EQUAL_INT(sizeof(buf), app_storage_write(buf, sizeof(buf), 0));
    // Change data with no CRC update
    buf[sizeof(buf) - 1] = 0xAB;
    nvm_write((void *) &app_storage_real.data, buf, sizeof(buf));
    // Ensure invalid CRC
    TEST_ASSERT_EQUAL_INT(APP_STORAGE_ERR_CORRUPTED, app_storage_init());
}

/* Test that the hook tells a first start, an intact storage and a lost one apart */
void test_init_callback(void)
{
    callback_calls = 0;

    // --- First start: no storage yet
    TEST_ASSERT_EQUAL_INT(APP_STORAGE_ERR_INVALID_HEADER, app_storage_init());
    TEST_ASSERT_EQUAL_INT(1, callback_calls);
    TEST_ASSERT_EQUAL_INT(APP_STORAGE_ERR_INVALID_HEADER, callback_status);

    // --- Intact storage
    uint8_t buf[20];
    memset(buf, 0xAA, sizeof(buf));
    TEST_ASSERT_EQUAL_INT(sizeof(buf), app_storage_write(buf, sizeof(buf), 0));
    TEST_ASSERT_EQUAL_INT(APP_STORAGE_SUCCESS, app_storage_init());
    TEST_ASSERT_EQUAL_INT(2, callback_calls);
    TEST_ASSERT_EQUAL_INT(APP_STORAGE_SUCCESS, callback_status);

    // --- Corrupted data: change data with no CRC update
    buf[sizeof(buf) - 1] = 0xAB;
    nvm_write((void *) &app_storage_real.data, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_INT(APP_STORAGE_ERR_CORRUPTED, app_storage_init());
    TEST_ASSERT_EQUAL_INT(3, callback_calls);
    TEST_ASSERT_EQUAL_INT(APP_STORAGE_ERR_CORRUPTED, callback_status);
    TEST_ASSERT_EQUAL_INT(0, app_storage_get_size());

    // --- The next initialization, such as one repeated after an IO reset in the same boot, finds
    // the reset storage intact: the hook gets APP_STORAGE_SUCCESS, so the loss is reported once
    TEST_ASSERT_EQUAL_INT(APP_STORAGE_SUCCESS, app_storage_init());
    TEST_ASSERT_EQUAL_INT(4, callback_calls);
    TEST_ASSERT_EQUAL_INT(APP_STORAGE_SUCCESS, callback_status);

    // --- Damaged tag of an initialized storage
    TEST_ASSERT_EQUAL_INT(sizeof(buf), app_storage_write(buf, sizeof(buf), 0));
    app_storage_header_t header = app_storage_real.header;
    header.tag[0] ^= 0x01;
    nvm_write((void *) &app_storage_real.header, &header, sizeof(header));
    TEST_ASSERT_EQUAL_INT(APP_STORAGE_ERR_CORRUPTED, app_storage_init());
    TEST_ASSERT_EQUAL_INT(5, callback_calls);
    TEST_ASSERT_EQUAL_INT(APP_STORAGE_ERR_CORRUPTED, callback_status);
    TEST_ASSERT_EQUAL_INT(0, app_storage_get_size());

    // --- Tag written in part on a fresh install: still a first start
    memset(&app_storage_real, 0, sizeof(app_storage_real));
    app_storage_real.header.tag[0] = APP_STORAGE_TAG[0];
    TEST_ASSERT_EQUAL_INT(APP_STORAGE_ERR_INVALID_HEADER, app_storage_init());
    TEST_ASSERT_EQUAL_INT(6, callback_calls);
    TEST_ASSERT_EQUAL_INT(APP_STORAGE_ERR_INVALID_HEADER, callback_status);

    // --- Each field past the tag tells an initialized storage from a fresh install
    const size_t fields[] = {offsetof(app_storage_t, crc),
                             offsetof(app_storage_t, header.size),
                             offsetof(app_storage_t, header.struct_version),
                             offsetof(app_storage_t, header.properties),
                             offsetof(app_storage_t, header.data_version)};
    for (size_t i = 0; i < sizeof(fields) / sizeof(fields[0]); i++) {
        memset(&app_storage_real, 0, sizeof(app_storage_real));
        ((uint8_t *) &app_storage_real)[fields[i]] = 0x01;
        TEST_ASSERT_EQUAL_INT(APP_STORAGE_ERR_CORRUPTED, app_storage_init());
    }
}

/* Test that a size past the storage is a corruption, and that a full storage is not */
void test_size_past_storage(void)
{
    setup_from_empty();

    // --- Full storage
    uint8_t *buf = malloc(APP_STORAGE_SIZE);
    TEST_ASSERT_NOT_NULL(buf);
    memset(buf, 0x55, APP_STORAGE_SIZE);
    TEST_ASSERT_EQUAL_INT(APP_STORAGE_SIZE, app_storage_write(buf, APP_STORAGE_SIZE, 0));
    free(buf);
    TEST_ASSERT_EQUAL_INT(APP_STORAGE_SUCCESS, app_storage_init());
    TEST_ASSERT_EQUAL_INT(APP_STORAGE_SIZE, app_storage_get_size());

    // --- One byte past the storage, and a size reaching out of the memory
    const uint32_t sizes[] = {APP_STORAGE_SIZE + 1, 0xFFFFFFFF};
    for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); i++) {
        nvm_write((void *) &app_storage_real.header.size, (void *) &sizes[i], sizeof(sizes[i]));
        TEST_ASSERT_EQUAL_INT(APP_STORAGE_ERR_CORRUPTED, app_storage_init());
        TEST_ASSERT_EQUAL_INT(APP_STORAGE_ERR_CORRUPTED, callback_status);
        TEST_ASSERT_EQUAL_INT(0, app_storage_get_size());
        TEST_ASSERT_EQUAL_INT(APP_STORAGE_SUCCESS, app_storage_init());
    }
}

/* Test that corruption from prepared storage is detected */
void test_corrupted_storage_from_prepared(void)
{
    setup_from_prepared();
    // --- Simulate corrupted header
    app_storage_header_t header = app_storage_real.header;
    header.data_version += 1;
    // Change header with no CRC update
    nvm_write((void *) &app_storage_real.header, &header, sizeof(header));
    // Ensure invalid CRC
    TEST_ASSERT_EQUAL_INT(APP_STORAGE_ERR_CORRUPTED, app_storage_init());

    // --- Simulate corrupted data
    setup_from_prepared();
    uint8_t data[INITIAL_SIZE + ADDITIONALL_SIZE] = {0};
    app_storage_read(data, INITIAL_SIZE + ADDITIONALL_SIZE, 0);
    // Change data with no CRC update
    data[INITIAL_SIZE + ADDITIONALL_SIZE - 1]++;
    nvm_write((void *) &app_storage_real.data, data, INITIAL_SIZE + ADDITIONALL_SIZE);
    // Ensure invalid CRC
    TEST_ASSERT_EQUAL_INT(APP_STORAGE_ERR_CORRUPTED, app_storage_init());
}

/* Read error cases with initially empty storage */
void test_read_error_from_empty(void)
{
    setup_from_empty();
    /* buf = NULL */
    TEST_ASSERT_EQUAL_INT(APP_STORAGE_ERR_INVALID_ARGUMENT, app_storage_read(NULL, 5, 0));

    /* nbytes = 0 */
    uint8_t buf[20];
    memset(buf, 0xAA, sizeof(buf));
    TEST_ASSERT_EQUAL_INT(APP_STORAGE_ERR_INVALID_ARGUMENT, app_storage_read(buf, 0, 0));

    /* size + offset integer overflow */
    TEST_ASSERT_EQUAL_INT(APP_STORAGE_ERR_INVALID_ARGUMENT,
                          app_storage_read(buf, UINT32_MAX - 4, 5));

    /* Reading 1 byte with 0 offset on empty app storage */
    TEST_ASSERT_EQUAL_INT(APP_STORAGE_ERR_NO_DATA_AVAILABLE, app_storage_read(buf, 1, 0));
}

/* Write error cases with initially empty storage */
void test_write_error_from_empty(void)
{
    setup_from_empty();
    /* buf = NULL */
    TEST_ASSERT_EQUAL_INT(APP_STORAGE_ERR_INVALID_ARGUMENT, app_storage_write(NULL, 5, 0));

    /* nbytes = 0 */
    uint8_t buf[20];
    memset(buf, 0xAA, sizeof(buf));
    TEST_ASSERT_EQUAL_INT(APP_STORAGE_ERR_INVALID_ARGUMENT, app_storage_write(buf, 0, 0));

    /* size + offset integer overflow */
    TEST_ASSERT_EQUAL_INT(APP_STORAGE_ERR_INVALID_ARGUMENT,
                          app_storage_write(buf, UINT32_MAX - 6, 7));

    /* Wring outside APP_STORAGE_SIZE */
    TEST_ASSERT_EQUAL_INT(APP_STORAGE_ERR_OVERFLOW,
                          app_storage_write(buf, APP_STORAGE_SIZE + 1, 0));

    /* Wring outside APP_STORAGE_SIZE */
    TEST_ASSERT_EQUAL_INT(APP_STORAGE_ERR_OVERFLOW, app_storage_write(buf, 1, APP_STORAGE_SIZE));
}

/* data_version combinations with initially empty storage */
void test_data_version_from_empty(void)
{
    setup_from_empty();
    /* The initial applicative data version is 1 */
    TEST_ASSERT_EQUAL_INT(APP_STORAGE_INITIAL_APP_DATA_VERSION, app_storage_get_data_version());

    /* The data version is manually incremented or set - not linked to writes */
    uint8_t buf_in[INITIAL_SIZE + ADDITIONALL_SIZE];
    for (uint32_t i = 0; i < sizeof(buf_in); i++) {
        buf_in[i] = i + 1;
    }
    TEST_ASSERT_EQUAL_INT(INITIAL_SIZE + ADDITIONALL_SIZE,
                          app_storage_write(buf_in, INITIAL_SIZE + ADDITIONALL_SIZE, 0));
    TEST_ASSERT_EQUAL_INT(APP_STORAGE_INITIAL_APP_DATA_VERSION, app_storage_get_data_version());

    /* Increment */
    app_storage_increment_data_version();
    TEST_ASSERT_EQUAL_INT(APP_STORAGE_INITIAL_APP_DATA_VERSION + 1, app_storage_get_data_version());
    app_storage_increment_data_version();
    TEST_ASSERT_EQUAL_INT(APP_STORAGE_INITIAL_APP_DATA_VERSION + 2, app_storage_get_data_version());

    /* Set */
    const uint32_t ver = 0xA5A53256;
    app_storage_set_data_version(ver);
    TEST_ASSERT_EQUAL_INT(ver, app_storage_get_data_version());
    app_storage_increment_data_version();
    TEST_ASSERT_EQUAL_INT(ver + 1, app_storage_get_data_version());

    /* Set maximum */
    app_storage_set_data_version(UINT32_MAX);
    TEST_ASSERT_EQUAL_INT(UINT32_MAX, app_storage_get_data_version());
    app_storage_increment_data_version();
    TEST_ASSERT_EQUAL_INT(APP_STORAGE_INITIAL_APP_DATA_VERSION, app_storage_get_data_version());
}

/* Write/read/get_size combinations with initially empty storage */
void test_write_read_from_empty(void)
{
    setup_from_empty();
    /* Checking data version */
    TEST_ASSERT_EQUAL_INT(APP_STORAGE_INITIAL_APP_DATA_VERSION, app_storage_get_data_version());

    uint8_t buf_in[INITIAL_SIZE + ADDITIONALL_SIZE];
    for (uint32_t i = 0; i < sizeof(buf_in); i++) {
        buf_in[i] = i + 1;
    }

    /* Normal write with 0 offset */
    /* 1, 2, 3 ... */
    TEST_ASSERT_EQUAL_INT(INITIAL_SIZE, app_storage_write(buf_in, INITIAL_SIZE, 0));
    app_storage_increment_data_version();

    /* Checking app storage size */
    TEST_ASSERT_EQUAL_INT(INITIAL_SIZE, app_storage_get_size());

    /* Reading the same bytes - normal case */
    uint8_t buf_out[INITIAL_SIZE + ADDITIONALL_SIZE] = {0};
    TEST_ASSERT_EQUAL_INT(INITIAL_SIZE, app_storage_read(buf_out, INITIAL_SIZE, 0));
    TEST_ASSERT_EQUAL_MEMORY(buf_in, buf_out, INITIAL_SIZE);

    /* Trying to read 1 byte more */
    TEST_ASSERT_EQUAL_INT(APP_STORAGE_ERR_NO_DATA_AVAILABLE,
                          app_storage_read(buf_out, INITIAL_SIZE + 1, 0));

    /* Trying to read correct size but with offset = 1 */
    TEST_ASSERT_EQUAL_INT(APP_STORAGE_ERR_NO_DATA_AVAILABLE,
                          app_storage_read(buf_out, INITIAL_SIZE, 1));

    /* Reading with offset = 1 */
    memset(buf_out, 0, sizeof(buf_out));
    TEST_ASSERT_EQUAL_INT(INITIAL_SIZE - 1, app_storage_read(buf_out, INITIAL_SIZE - 1, 1));
    TEST_ASSERT_EQUAL_MEMORY(&buf_in[1], buf_out, INITIAL_SIZE - 1);

    /* Next write */
    /* 1, 2, 3 ... 21, 22, 23 */
    TEST_ASSERT_EQUAL_INT(ADDITIONALL_SIZE,
                          app_storage_write(&buf_in[INITIAL_SIZE], ADDITIONALL_SIZE, INITIAL_SIZE));
    app_storage_increment_data_version();

    /* Checking app storage size */
    TEST_ASSERT_EQUAL_INT(INITIAL_SIZE + ADDITIONALL_SIZE, app_storage_get_size());

    /* Reading all */
    memset(buf_out, 0, sizeof(buf_out));
    TEST_ASSERT_EQUAL_INT(INITIAL_SIZE + ADDITIONALL_SIZE,
                          app_storage_read(buf_out, INITIAL_SIZE + ADDITIONALL_SIZE, 0));
    TEST_ASSERT_EQUAL_MEMORY(buf_in, &buf_out, INITIAL_SIZE + ADDITIONALL_SIZE);

    /* Reading at offset = INITIAL_SIZE */
    memset(buf_out, 0, sizeof(buf_out));
    TEST_ASSERT_EQUAL_INT(ADDITIONALL_SIZE,
                          app_storage_read(&buf_out[INITIAL_SIZE], ADDITIONALL_SIZE, INITIAL_SIZE));
    TEST_ASSERT_EQUAL_MEMORY(&buf_in[INITIAL_SIZE], &buf_out[INITIAL_SIZE], ADDITIONALL_SIZE);

    /* Rewriting in the middle and checking */
    /* 1, 2, 3 ... 20 + 0xA5, 21 + 0xA5, 22 + 0xA5 */
    for (uint32_t i = 0; i < sizeof(buf_in); i++) {
        buf_in[i] = i + 0xA5;
    }
    TEST_ASSERT_EQUAL_INT(INITIAL_SIZE, app_storage_write(buf_in, INITIAL_SIZE, 0));
    app_storage_increment_data_version();

    /* Checking app storage size */
    TEST_ASSERT_EQUAL_INT(INITIAL_SIZE + ADDITIONALL_SIZE, app_storage_get_size());

    /* Reading the same bytes - normal case */
    memset(buf_out, 0, sizeof(buf_out));
    TEST_ASSERT_EQUAL_INT(INITIAL_SIZE, app_storage_read(buf_out, INITIAL_SIZE, 0));
    TEST_ASSERT_EQUAL_MEMORY(buf_in, buf_out, INITIAL_SIZE);

    /* Checking data version */
    TEST_ASSERT_EQUAL_INT(APP_STORAGE_INITIAL_APP_DATA_VERSION + 3, app_storage_get_data_version());
}

void test_write_big_reset_from_empty(void)
{
    setup_from_empty();
    /* Checking data version */
    TEST_ASSERT_EQUAL_INT(APP_STORAGE_INITIAL_APP_DATA_VERSION, app_storage_get_data_version());

    /* Checking data size */
    TEST_ASSERT_EQUAL_INT(0, app_storage_get_size());
    uint32_t i   = 0;
    uint8_t  buf = 0;
    for (; i < APP_STORAGE_SIZE; i++) {
        buf = (uint8_t) i;
        TEST_ASSERT_EQUAL_INT(1, app_storage_write(&buf, 1, i));
        app_storage_increment_data_version();
        TEST_ASSERT_EQUAL_INT(APP_STORAGE_INITIAL_APP_DATA_VERSION + i + 1,
                              app_storage_get_data_version());
        TEST_ASSERT_EQUAL_INT(i + 1, app_storage_get_size());
    }

    /* Cannot write more */
    buf = (uint8_t) i;
    TEST_ASSERT_EQUAL_INT(APP_STORAGE_ERR_OVERFLOW, app_storage_write(&buf, 1, i));
    TEST_ASSERT_EQUAL_INT(APP_STORAGE_INITIAL_APP_DATA_VERSION + i, app_storage_get_data_version());
    TEST_ASSERT_EQUAL_INT(i, app_storage_get_size());

    /* Read */
    i   = 0;
    buf = 0;
    for (; i < APP_STORAGE_SIZE; i++) {
        TEST_ASSERT_EQUAL_INT(1, app_storage_read(&buf, 1, i));
        TEST_ASSERT_EQUAL_INT((uint8_t) i, buf);
    }

    /* Reset */
    app_storage_reset();
    /* Checking data version */
    TEST_ASSERT_EQUAL_INT(APP_STORAGE_INITIAL_APP_DATA_VERSION, app_storage_get_data_version());
    /* Checking data size */
    TEST_ASSERT_EQUAL_INT(0, app_storage_get_size());
    /* Checking properties */
    TEST_ASSERT_EQUAL_INT(APP_STORAGE_PROP_SETTINGS | APP_STORAGE_PROP_DATA,
                          app_storage_get_properties());

    /* Read is not possible */
    buf = 0;
    TEST_ASSERT_EQUAL_INT(APP_STORAGE_ERR_NO_DATA_AVAILABLE, app_storage_read(&buf, 1, 0));

    /* Tricky way to read all storage */
    buf = 0;
    TEST_ASSERT_EQUAL_INT(1, app_storage_write(&buf, 1, APP_STORAGE_SIZE - 1));
    TEST_ASSERT_EQUAL_INT(APP_STORAGE_SIZE, app_storage_get_size());

    /* All zeroes at read */
    i   = 0;
    buf = 0;
    for (; i < APP_STORAGE_SIZE; i++) {
        TEST_ASSERT_EQUAL_INT(1, app_storage_read(&buf, 1, i));
        TEST_ASSERT_EQUAL_INT(0, buf);
    }
}

void test_write_read_from_prepared(void)
{
    setup_from_prepared();
    /* Checking data version */
    TEST_ASSERT_EQUAL_INT(APP_STORAGE_INITIAL_APP_DATA_VERSION + 3, app_storage_get_data_version());

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
    TEST_ASSERT_EQUAL_INT(INITIAL_SIZE + ADDITIONALL_SIZE,
                          app_storage_read(buf_out, INITIAL_SIZE + ADDITIONALL_SIZE, 0));
    TEST_ASSERT_EQUAL_MEMORY(buf_in, buf_out, INITIAL_SIZE + ADDITIONALL_SIZE);

    /* Write overpassing and overplaping, read and compare */
    /* [1, 2, 3 ... 19] [20 + 0x33, 5 + 0x33, 6 + 0x33, 7 + 0x33, 8 + 0x33 ... 559 + 0x33]*/
    for (uint32_t i = INITIAL_SIZE; i < INITIAL_SIZE + ADDITIONALL_SIZE2; i++) {
        buf_in[i] = i + 0x33;
    }
    TEST_ASSERT_EQUAL_INT(
        ADDITIONALL_SIZE2,
        app_storage_write(&buf_in[INITIAL_SIZE], ADDITIONALL_SIZE2, INITIAL_SIZE));
    app_storage_increment_data_version();

    TEST_ASSERT_EQUAL_INT(INITIAL_SIZE + ADDITIONALL_SIZE2, app_storage_get_size());

    memset(buf_out, 0, INITIAL_SIZE + ADDITIONALL_SIZE2);
    TEST_ASSERT_EQUAL_INT(INITIAL_SIZE + ADDITIONALL_SIZE2,
                          app_storage_read(buf_out, INITIAL_SIZE + ADDITIONALL_SIZE2, 0));
    TEST_ASSERT_EQUAL_MEMORY(buf_in, buf_out, INITIAL_SIZE + ADDITIONALL_SIZE2);

    /* Checking data version */
    TEST_ASSERT_EQUAL_INT(APP_STORAGE_INITIAL_APP_DATA_VERSION + 4, app_storage_get_data_version());

    /* Writing at big offset */
    uint8_t buf_in3[INITIAL_SIZE + ADDITIONALL_SIZE3] = {0};
    memcpy(buf_in3, buf_in, INITIAL_SIZE + ADDITIONALL_SIZE2);
    buf_in3[INITIAL_SIZE + ADDITIONALL_SIZE3 - 1] = 0xB6;

    TEST_ASSERT_EQUAL_INT(1,
                          app_storage_write(&buf_in3[INITIAL_SIZE + ADDITIONALL_SIZE3 - 1],
                                            1,
                                            INITIAL_SIZE + ADDITIONALL_SIZE3 - 1));
    app_storage_increment_data_version();
    TEST_ASSERT_EQUAL_INT(INITIAL_SIZE + ADDITIONALL_SIZE3, app_storage_get_size());

    uint8_t buf_out3[INITIAL_SIZE + ADDITIONALL_SIZE3] = {0};

    TEST_ASSERT_EQUAL_INT(INITIAL_SIZE + ADDITIONALL_SIZE3,
                          app_storage_read(buf_out3, INITIAL_SIZE + ADDITIONALL_SIZE3, 0));
    TEST_ASSERT_EQUAL_MEMORY(buf_in3, buf_out3, INITIAL_SIZE + ADDITIONALL_SIZE3);

    /* Checking data version */
    TEST_ASSERT_EQUAL_INT(APP_STORAGE_INITIAL_APP_DATA_VERSION + 5, app_storage_get_data_version());
}

void test_app_style_from_empty(void)
{
    setup_from_empty();
    /* Checking data version */
    TEST_ASSERT_EQUAL_INT(APP_STORAGE_INITIAL_APP_DATA_VERSION, app_storage_get_data_version());
    /* Checking data size */
    TEST_ASSERT_EQUAL_INT(0, app_storage_get_size());

    uint32_t version     = 0x01;
    uint8_t  initialized = 1;
    TEST_ASSERT_EQUAL_INT(sizeof(version), APP_STORAGE_WRITE_F(version, &version));
    TEST_ASSERT_EQUAL_INT(sizeof(initialized), APP_STORAGE_WRITE_F(initialized, &initialized));
    app_storage_increment_data_version();

    for (uint32_t i = 0; i < SLOT_NUMBER; i++) {
        slot_t slot = {
            0xA454B0C5, {(i + 1) * 1, (i + 1) * 2, (i + 1) * 3}
        };
        TEST_ASSERT_EQUAL_INT(sizeof(slot), APP_STORAGE_WRITE_F(slot[i], &slot));
        app_storage_increment_data_version();
    }

    uint32_t slot_number = SLOT_NUMBER;
    TEST_ASSERT_EQUAL_INT(sizeof(slot_number), APP_STORAGE_WRITE_F(slot_number, &slot_number));
}

void test_app_style_from_prepared(void)
{
    setup_from_prepared_app_style();
    /* Checking data version */
    TEST_ASSERT_EQUAL_INT(APP_STORAGE_INITIAL_APP_DATA_VERSION + 1 + SLOT_NUMBER,
                          app_storage_get_data_version());
    /* Checking data size */
    TEST_ASSERT_EQUAL_INT(sizeof(app_storage_data_t), app_storage_get_size());

    uint32_t version     = 0;
    uint8_t  initialized = 0;
    uint32_t slot_number = 0;
    TEST_ASSERT_EQUAL_INT(sizeof(version), APP_STORAGE_READ_F(version, &version));
    TEST_ASSERT_EQUAL_INT(0x01, version);
    TEST_ASSERT_EQUAL_INT(sizeof(initialized), APP_STORAGE_READ_F(initialized, &initialized));
    TEST_ASSERT_EQUAL_INT(1, initialized);
    TEST_ASSERT_EQUAL_INT(sizeof(slot_number), APP_STORAGE_READ_F(slot_number, &slot_number));
    TEST_ASSERT_EQUAL_INT(SLOT_NUMBER, slot_number);

    for (uint32_t i = 0; i < SLOT_NUMBER; i++) {
        const slot_t slot_c = {
            0xA454B0C5, {(i + 1) * 1, (i + 1) * 2, (i + 1) * 3}
        };
        slot_t slot = {0};
        TEST_ASSERT_EQUAL_INT(sizeof(slot), APP_STORAGE_READ_F(slot[i], &slot));
        TEST_ASSERT_EQUAL_MEMORY(&slot_c, &slot, sizeof(slot));
    }
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_getters_from_empty);
    RUN_TEST(test_corrupted_storage_from_empty);
    RUN_TEST(test_init_callback);
    RUN_TEST(test_size_past_storage);
    RUN_TEST(test_read_error_from_empty);
    RUN_TEST(test_write_error_from_empty);
    RUN_TEST(test_data_version_from_empty);
    RUN_TEST(test_write_read_from_empty);
    RUN_TEST(test_write_big_reset_from_empty);
    RUN_TEST(test_write_read_from_prepared);
    RUN_TEST(test_corrupted_storage_from_prepared);
    RUN_TEST(test_app_style_from_empty);
    RUN_TEST(test_app_style_from_prepared);
    return UNITY_END();
}
