#include <stdarg.h>

#include "mocks.h"

uint8_t        fuzz_ctrl[FUZZ_CTRL_SIZE];
const uint8_t *fuzz_tail_ptr = NULL;
size_t         fuzz_tail_len = 0;

/* PRINTF is excluded from the fuzz macro set, so app sources call it as a
 * function. Keep this stub or replace it with an app-specific logger. */
int PRINTF(const char *format, ...)
{
    (void) format;
    return 0;
}

/* Zeroing BSS would erase Absolution prefix state between iterations. */
void os_explicit_zero_BSS_segment(void) {}
