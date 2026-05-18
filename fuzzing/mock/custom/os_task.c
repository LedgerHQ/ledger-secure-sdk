#include "exceptions.h"
#include <stdio.h>
#include <stdint.h>

struct {
    uint8_t bss[0xff];
    uint8_t ebss[0xff];
} mock_memory;

void *_bss  = &mock_memory.bss;
void *_ebss = &mock_memory.ebss;
