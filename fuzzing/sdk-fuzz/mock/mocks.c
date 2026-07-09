#include "mocks.h"

uint8_t        fuzz_ctrl[FUZZ_CTRL_SIZE];
const uint8_t *fuzz_tail_ptr = NULL;
size_t         fuzz_tail_len = 0;

void os_explicit_zero_BSS_segment(void) {}
