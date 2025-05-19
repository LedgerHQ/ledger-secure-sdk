#include "exceptions.h"
#include <stdio.h>
#include <stdint.h>

try_context_t *try_context_get(void);

try_context_t *try_context_set(try_context_t *context);
