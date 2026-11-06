#pragma once

#include "bolos_target.h"
#include "decorators.h"

// halt the chip, waiting for a physical user interaction
__attribute__((noreturn)) SYSCALL void halt(void);

// deprecated
#define reset halt
