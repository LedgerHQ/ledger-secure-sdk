#include "mocks.h"
#include "buffer.h"

uint8_t        fuzz_ctrl[FUZZ_CTRL_SIZE];
const uint8_t *fuzz_tail_ptr = NULL;
size_t         fuzz_tail_len = 0;

void os_explicit_zero_BSS_segment(void) {}

/* io.c defines io_send_response_buffers as WEAK, but --allow-multiple-definition
 * makes LLD use the first definition (first-wins), so io.c wins over any stub in
 * a harness .c file linked later.  Putting the stub here (mocks.c.o is the first
 * object in every fuzz target's link command) guarantees the no-op wins. */
int io_send_response_buffers(const buffer_t *rdatalist, size_t count, uint16_t sw)
{
    (void) rdatalist;
    (void) count;
    (void) sw;
    return 0;
}
