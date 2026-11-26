// Device symbols referenced by src/os.c, unused by the tested helpers

#include <stddef.h>
#include <stdint.h>

#include "os_io_seph_cmd.h"

void *_bss;
void *_ebss;

void cx_rng_no_throw(uint8_t *buffer, size_t len)
{
    (void) buffer;
    (void) len;
}

int os_io_seph_cmd_se_reset(void)
{
    return 0;
}
