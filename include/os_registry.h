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
