// #include "os_task.h"
#include "exceptions.h"
#include <stdio.h>
#include <stdint.h>

try_context_t *try_context_get(void);

try_context_t *try_context_set(try_context_t *context);

static try_context_t *G_exception_context = NULL;

try_context_t *try_context_get(void)
{
    return G_exception_context;
}

try_context_t *try_context_set(try_context_t *context)
{
    try_context_t *previous = G_exception_context;
    G_exception_context     = context;
    return previous;
}

struct {
    uint8_t bss[0xff];
    uint8_t ebss[0xff];
} mock_memory;

void *_bss  = &mock_memory.bss;
void *_ebss = &mock_memory.ebss;
