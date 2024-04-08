#pragma once

#include <stdio.h>
#include "app_config.h"

// avoid typing the size each time
#ifdef SPRINTF
#undef SPRINTF
#endif  // SPRINTF
#define SPRINTF(strbuf, ...) snprintf(strbuf, sizeof(strbuf), __VA_ARGS__)

#ifdef HAVE_PRINTF
void screen_printf(const char *format, ...);
void mcu_usb_printf(const char *format, ...);
#else  // !HAVE_PRINTF
#define PRINTF(...)
#endif  // !HAVE_PRINTF
