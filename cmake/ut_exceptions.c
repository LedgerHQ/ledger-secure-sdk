// Host implementation of the SDK exception primitives, for TRY / CATCH / THROW in unit tests.
// Symbols are weak: a test can define its own.

#include <stdlib.h>

#include "exceptions.h"
#include "unity.h"

static try_context_t *current_context = NULL;
// Unity test during which current_context was set
static UNITY_COUNTER_TYPE context_test = 0;

// A test aborted by Unity inside a TRY skips CLOSE_TRY: drop its context in the next test
static void drop_stale_context(void)
{
    if (Unity.NumberOfTests != context_test) {
        current_context = NULL;
    }
}

__attribute__((weak)) try_context_t *try_context_get(void)
{
    drop_stale_context();
    return current_context;
}

__attribute__((weak)) try_context_t *try_context_set(try_context_t *context)
{
    drop_stale_context();
    try_context_t *previous = current_context;
    current_context         = context;
    context_test            = Unity.NumberOfTests;
    return previous;
}

__attribute__((weak)) void os_longjmp(unsigned int exception)
{
    try_context_t *context = try_context_get();

    if (context == NULL) {
        TEST_FAIL_MESSAGE("Exception thrown outside of a TRY block");
        abort();
    }
    longjmp(context->jmp_buf, exception);
}
