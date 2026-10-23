#include <stdbool.h>

#include "unity.h"

#include "exceptions.h"

#define EXCEPTION_A 0x6A80
#define EXCEPTION_B 0x6985

void setUp(void) {}

void tearDown(void) {}

void test_throw_is_caught(void)
{
    volatile bool after_throw = false;
    volatile bool caught      = false;
    volatile bool finally     = false;

    BEGIN_TRY
    {
        TRY
        {
            THROW(EXCEPTION_A);
            after_throw = true;
        }
        CATCH(EXCEPTION_A)
        {
            caught = true;
        }
        FINALLY
        {
            finally = true;
        }
    }
    END_TRY;

    TEST_ASSERT_FALSE(after_throw);
    TEST_ASSERT_TRUE(caught);
    TEST_ASSERT_TRUE(finally);
    TEST_ASSERT_NULL(try_context_get());
}

void test_no_throw_restores_context(void)
{
    volatile bool caught  = false;
    volatile bool finally = false;

    BEGIN_TRY
    {
        TRY
        {
            TEST_ASSERT_NOT_NULL(try_context_get());
        }
        CATCH_ALL
        {
            caught = true;
        }
        FINALLY
        {
            finally = true;
        }
    }
    END_TRY;

    TEST_ASSERT_FALSE(caught);
    TEST_ASSERT_TRUE(finally);
    TEST_ASSERT_NULL(try_context_get());
}

void test_uncaught_exception_reaches_outer_try(void)
{
    volatile bool        inner_caught  = false;
    volatile bool        inner_finally = false;
    volatile exception_t outer_caught  = 0;

    BEGIN_TRY_L(outer)
    {
        TRY_L(outer)
        {
            BEGIN_TRY_L(inner)
            {
                TRY_L(inner)
                {
                    THROW(EXCEPTION_A);
                }
                CATCH_L(inner, EXCEPTION_B)
                {
                    inner_caught = true;
                }
                FINALLY_L(inner)
                {
                    inner_finally = true;
                }
            }
            END_TRY_L(inner);
        }
        CATCH_OTHER_L(outer, e)
        {
            outer_caught = e;
        }
        FINALLY_L(outer) {}
    }
    END_TRY_L(outer);

    TEST_ASSERT_FALSE(inner_caught);
    TEST_ASSERT_TRUE(inner_finally);
    TEST_ASSERT_EQUAL_HEX16(EXCEPTION_A, outer_caught);
    TEST_ASSERT_NULL(try_context_get());
}

// Run in this order: the first test leaves its TRY context behind, the second checks it is gone
void test_abort_inside_try(void)
{
    BEGIN_TRY
    {
        TRY
        {
            TEST_IGNORE_MESSAGE("Aborts the test inside a TRY block");
        }
        FINALLY {}
    }
    END_TRY;
}

void test_no_context_after_abort_inside_try(void)
{
    TEST_ASSERT_NULL(try_context_get());
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_throw_is_caught);
    RUN_TEST(test_no_throw_restores_context);
    RUN_TEST(test_uncaught_exception_reaches_outer_try);
    RUN_TEST(test_abort_inside_try);
    RUN_TEST(test_no_context_after_abort_inside_try);
    return UNITY_END();
}
