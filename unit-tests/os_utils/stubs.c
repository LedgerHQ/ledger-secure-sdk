// Device symbols referenced by src/os.c, unused by the tested helpers

#include <stddef.h>
#include <stdint.h>

#include "exceptions.h"
#include "os_io_seph_cmd.h"

void *_bss;
void *_ebss;

void cx_rng_no_throw(uint8_t *buffer, size_t len)
{
    (void) buffer;
    (void) len;
}

try_context_t *try_context_get(void)
{
    return NULL;
}

try_context_t *try_context_set(try_context_t *context)
{
    (void) context;
    return NULL;
}

int os_io_seph_cmd_se_reset(void)
{
    return 0;
}
