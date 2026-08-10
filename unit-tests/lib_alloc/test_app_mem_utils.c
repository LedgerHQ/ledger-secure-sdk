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

/**
 * @brief Unit tests for app_mem_utils.c (one binary, one compilation unit).
 *
 * mem_alloc.h is mocked via CMock so that only the logic inside
 * app_mem_utils.c is exercised: delegation, null-skipping, zero-size early
 * return, nullification after free, and string duplication.
 */

#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "unity.h"
#include "Mockmem_alloc.h"
#include "app_mem_utils.h"

static void   *g_fake_ctx = (void *) 0xCA11AB1E;
static void   *g_fake_ptr = (void *) 0xBEEF0000;
static uint8_t g_dummy_heap[256];

void setUp(void)
{
    Mockmem_alloc_Init();
    mem_init_IgnoreAndReturn(g_fake_ctx);
    mem_utils_init(g_dummy_heap, sizeof(g_dummy_heap));
}

void tearDown(void)
{
    Mockmem_alloc_Verify();
    Mockmem_alloc_Destroy();
}

void test_init_returns_true_when_mem_init_succeeds(void)
{
    mem_init_IgnoreAndReturn(g_fake_ctx);
    TEST_ASSERT_TRUE(mem_utils_init(g_dummy_heap, sizeof(g_dummy_heap)));
}

void test_init_returns_false_when_mem_init_fails(void)
{
    mem_init_IgnoreAndReturn(NULL);
    TEST_ASSERT_FALSE(mem_utils_init(g_dummy_heap, sizeof(g_dummy_heap)));
}

void test_alloc_delegates_to_mem_alloc(void)
{
    mem_alloc_ExpectAndReturn(g_fake_ctx, 64, g_fake_ptr);
    TEST_ASSERT_EQUAL_PTR(g_fake_ptr, APP_MEM_ALLOC(64));
}

void test_alloc_returns_null_on_failure(void)
{
    mem_alloc_ExpectAndReturn(g_fake_ctx, 64, NULL);
    TEST_ASSERT_NULL(APP_MEM_ALLOC(64));
}

void test_free_skips_null_ptr(void)
{
    /* No mem_free expectation — CMock fails if mem_free is called unexpectedly. */
    APP_MEM_FREE(NULL);
}

void test_free_delegates_to_mem_free(void)
{
    mem_free_Expect(g_fake_ctx, g_fake_ptr);
    APP_MEM_FREE(g_fake_ptr);
}

void test_calloc_zero_size_returns_true_without_alloc(void)
{
    /* Implementation returns true immediately without touching *buffer. */
    void *buf = (void *) 0xDEAD;
    TEST_ASSERT_TRUE(APP_MEM_CALLOC(&buf, 0));
    TEST_ASSERT_EQUAL_PTR((void *) 0xDEAD, buf);
}

void test_calloc_zeros_allocated_buffer(void)
{
    static uint8_t fake_block[32];
    memset(fake_block, 0xFF, sizeof(fake_block));
    mem_alloc_ExpectAndReturn(g_fake_ctx, sizeof(fake_block), fake_block);
    void *buf = NULL;
    TEST_ASSERT_TRUE(APP_MEM_CALLOC(&buf, sizeof(fake_block)));
    TEST_ASSERT_EQUAL_PTR(fake_block, buf);
    for (size_t i = 0; i < sizeof(fake_block); i++) {
        TEST_ASSERT_EQUAL_INT(0, fake_block[i]);
    }
}

void test_calloc_returns_false_on_alloc_failure(void)
{
    mem_alloc_ExpectAndReturn(g_fake_ctx, 32, NULL);
    void *buf = NULL;
    TEST_ASSERT_FALSE(APP_MEM_CALLOC(&buf, 32));
    TEST_ASSERT_NULL(buf);
}

void test_free_and_null_nullifies_pointer(void)
{
    void *buf = g_fake_ptr;
    mem_free_Expect(g_fake_ctx, g_fake_ptr);
    APP_MEM_FREE_AND_NULL(&buf);
    TEST_ASSERT_NULL(buf);
}

void test_free_and_null_skips_null_ptr(void)
{
    /* No mem_free expectation. */
    void *buf = NULL;
    APP_MEM_FREE_AND_NULL(&buf);
    TEST_ASSERT_NULL(buf);
}

void test_realloc_delegates_to_mem_realloc(void)
{
    mem_realloc_ExpectAndReturn(g_fake_ctx, g_fake_ptr, 128, g_fake_ptr);
    TEST_ASSERT_EQUAL_PTR(g_fake_ptr, APP_MEM_REALLOC(g_fake_ptr, 128));
}

void test_strdup_copies_string(void)
{
    static char fake_dst[16];
    const char *src = "hello";
    mem_alloc_ExpectAndReturn(g_fake_ctx, strlen(src) + 1, fake_dst);
    const char *dup = APP_MEM_STRDUP(src);
    TEST_ASSERT_EQUAL_PTR(fake_dst, dup);
    TEST_ASSERT_EQUAL_STRING(src, fake_dst);
}

void test_strdup_returns_null_on_alloc_failure(void)
{
    mem_alloc_ExpectAndReturn(g_fake_ctx, strlen("hello") + 1, NULL);
    TEST_ASSERT_NULL(APP_MEM_STRDUP("hello"));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_init_returns_true_when_mem_init_succeeds);
    RUN_TEST(test_init_returns_false_when_mem_init_fails);
    RUN_TEST(test_alloc_delegates_to_mem_alloc);
    RUN_TEST(test_alloc_returns_null_on_failure);
    RUN_TEST(test_free_skips_null_ptr);
    RUN_TEST(test_free_delegates_to_mem_free);
    RUN_TEST(test_calloc_zero_size_returns_true_without_alloc);
    RUN_TEST(test_calloc_zeros_allocated_buffer);
    RUN_TEST(test_calloc_returns_false_on_alloc_failure);
    RUN_TEST(test_free_and_null_nullifies_pointer);
    RUN_TEST(test_free_and_null_skips_null_ptr);
    RUN_TEST(test_realloc_delegates_to_mem_realloc);
    RUN_TEST(test_strdup_copies_string);
    RUN_TEST(test_strdup_returns_null_on_alloc_failure);
    return UNITY_END();
}
