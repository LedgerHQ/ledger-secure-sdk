#include "exceptions.h"
#include <stdio.h>
#include <stdint.h>

struct {
    uint8_t bss[0xff];
    uint8_t ebss[0xff];

} mock_memory;

try_context_t *try_context_get(void);

try_context_t *try_context_set(try_context_t *context);
