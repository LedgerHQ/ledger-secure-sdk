/*
 * Absolution requires at least one translation unit in TARGETS to parse for
 * globals, and the SDK self-fuzz targets have no app sources of their own -- the
 * harness is passed separately as HARNESS.
 */

#include "mocks.h"
#include "buffer.h"

/* io.c defines io_send_response_buffers as WEAK and --allow-multiple-definition
 * makes LLD take the first definition, so this stub has to sit in the first
 * object of the link command -- which is mocks.c.o, not the shared runtime. */
int io_send_response_buffers(const buffer_t *rdatalist, size_t count, uint16_t sw)
{
    (void) rdatalist;
    (void) count;
    (void) sw;
    return 0;
}
