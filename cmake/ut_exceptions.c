// Host implementation of the SDK exception primitives, for TRY / CATCH / THROW in unit tests.
// Symbols are weak: a test can define its own.

#include <stdlib.h>

#include "exceptions.h"
#include "unity.h"

static try_context_t *current_context = NULL;

__attribute__((weak)) try_context_t *try_context_get(void)
{
    return current_context;
}

__attribute__((weak)) try_context_t *try_context_set(try_context_t *context)
{
    try_context_t *previous = current_context;
    current_context         = context;
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
