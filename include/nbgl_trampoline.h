#pragma once

#include "bolos_target.h"

#if defined(TARGET_NANOX)
#define NBGL_TRAMPOLINE_ADDR 0x00210001
#elif defined(TARGET_NANOS2)
#define NBGL_TRAMPOLINE_ADDR 0x00810001
#elif defined(TARGET_STAX)
#define NBGL_TRAMPOLINE_ADDR 0x00818001
#elif defined(TARGET_FLEX)
#define NBGL_TRAMPOLINE_ADDR 0x0081E001
#else
#error "Not supported TARGET"
#endif
