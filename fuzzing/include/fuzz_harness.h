#pragma once
/*
 * Generic Absolution APDU harness.
 *
 * Provides fuzz_harness_entry(), the default body for the app's fuzz_entry():
 * Absolution restores the invariant prefix, then this harness dispatches one
 * APDU per iteration built from the fuzz tail.
 *
 * Required app-provided symbols:
 * - fuzz_commands[]
 * - fuzz_n_commands
 * - fuzz_app_reset()
 * - fuzz_app_dispatch(void *cmd)
 *
 * Required from the app's mocks.h before including this header:
 * - try_context_t fuzz_exit_jump_ctx
 * - uint8_t fuzz_ctrl[FUZZ_CTRL_SIZE]
 * - const uint8_t *fuzz_tail_ptr
 * - size_t fuzz_tail_len
 * - FUZZ_CTRL_SIZE
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <setjmp.h>
#include "fuzz_defs.h"
#include "mocks.h"
#include "parser.h"

extern const fuzz_command_spec_t fuzz_commands[];
extern const size_t              fuzz_n_commands;
extern void                      fuzz_app_reset(void);
extern void                      fuzz_app_dispatch(void *cmd);

static inline int fuzz_use_structured_lane(void)
{
    return fuzz_ctrl[0] > FUZZ_STRUCTURED_LANE_THRESHOLD;
}

static inline uint8_t fuzz_clamp_p(uint8_t raw, uint8_t p_max)
{
    if (p_max == 0) {
        return raw;
    }
    return raw % (p_max + 1);
}

/* Apps may override these macros before including this header to install
 * lane-specific command tables. */
#ifndef FUZZ_PICK_COMMAND_STRUCTURED
#define FUZZ_PICK_COMMAND_STRUCTURED(data, size) (&fuzz_commands[fuzz_ctrl[1] % fuzz_n_commands])
#endif

#ifndef FUZZ_PICK_COMMAND_RAW
#define FUZZ_PICK_COMMAND_RAW(data, size) (&fuzz_commands[(data)[1] % fuzz_n_commands])
#endif

static int fuzz_harness_entry(const uint8_t *data, size_t size)
{
    if (sigsetjmp(fuzz_exit_jump_ctx.jmp_buf, 1)) {
        return 0;
    }

    if (size < 4 || fuzz_n_commands == 0) {
        return -1;
    }

    fuzz_app_reset();

    const fuzz_command_spec_t *spec;

    if (fuzz_use_structured_lane()) {
        fuzz_tail_ptr = (size > 4) ? data + 4 : NULL;
        fuzz_tail_len = (size > 4) ? size - 4 : 0;
        spec          = FUZZ_PICK_COMMAND_STRUCTURED(data, size);
    }
    else {
        fuzz_tail_ptr = NULL;
        fuzz_tail_len = 0;
        spec          = FUZZ_PICK_COMMAND_RAW(data, size);
    }

    command_t cmd;
    memset(&cmd, 0, sizeof(cmd));

    cmd.cla = spec->cla;
    cmd.ins = spec->ins;
    cmd.p1  = fuzz_clamp_p(data[2], spec->p1_max);
    cmd.p2  = fuzz_clamp_p(data[3], spec->p2_max);

    if (size > 4) {
        size_t payload = size - 4;
        cmd.lc         = (uint8_t) (payload > 255 ? 255 : payload);
        cmd.data       = (uint8_t *) &data[4];
    }

    fuzz_app_dispatch(&cmd);
    return 0;
}
