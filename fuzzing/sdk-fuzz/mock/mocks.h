#pragma once

#include <stddef.h>
#include <stdint.h>

#include "fuzz_defs.h"
#include "exceptions.h"

extern try_context_t fuzz_exit_jump_ctx;

extern const uint8_t *fuzz_tail_ptr;
extern size_t         fuzz_tail_len;
