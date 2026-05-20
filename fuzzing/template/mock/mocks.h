#pragma once

#include <stddef.h>
#include <stdint.h>

#include "fuzz_defs.h"
#include "exceptions.h"

/* Required: defined by the SDK os_exceptions mock; declared here because
 * fuzz_harness.h expects it at link time. */
extern try_context_t fuzz_exit_jump_ctx;

/* Required: control region the harness uses to shape one APDU. */
#define FUZZ_CTRL_SIZE 16
extern uint8_t        fuzz_ctrl[FUZZ_CTRL_SIZE];
extern const uint8_t *fuzz_tail_ptr;
extern size_t         fuzz_tail_len;
