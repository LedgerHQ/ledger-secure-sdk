#pragma once

#include "appflags.h"
#include "decorators.h"
#include "os_app.h"

/* ----------------------------------------------------------------------- */
/* -                          REGISTRY-RELATED                           - */
/* ----------------------------------------------------------------------- */
// read from the application's install parameter TLV (if present), else 0 is returned (no exception
// thrown). takes into account the currently loaded application
#define OS_REGISTRY_GET_TAG_OFFSET_COMPARE_WITH_BUFFER (0x80000000UL)
#define OS_REGISTRY_GET_TAG_OFFSET_GET_LENGTH          (0x40000000UL)

// Copy the currently running application tag from its install parameters to the given user buffer.
// Only APPNAME/APPVERSION/DERIVEPATH/ICON tags are retrievable to avoid install parameters private
// information. Warning: this function returns tag content of the application lastly scheduled to
// run, not the tag content of the currently executed piece of code (libraries subcalls)
SYSCALL unsigned int os_registry_get_current_app_tag(unsigned int          tag,
                                                     unsigned char *buffer PLENGTH(maxlen),
                                                     unsigned int          maxlen);
