#pragma once
/**
 * @file fuzz_harness.h
 * @brief Default APDU harness body for a fuzz target.
 *
 * Including this header gives a target the standard @c fuzz_harness_entry()
 * implementation: it turns one fuzzer input into one APDU and dispatches it
 * through the app. The app contract is small and entirely about the app:
 *
 *     const fuzz_command_spec_t fuzz_commands[] = { ... };
 *     FUZZ_COMMAND_COUNT();            // derives fuzz_n_commands
 *     void fuzz_app_reset(void);       // per-iteration state setup
 *     void fuzz_app_dispatch(void *);  // hand the built command_t to the app
 *
 * @c fuzz_app_cleanup() is optional: the framework ships a weak no-op default
 * that an app may override. Lane selection, command selection, P1/P2 clamping
 * and the custom mutator all come from the framework. See fuzz_defs.h for the
 * input layout — the harness takes its control bytes from the start of its own
 * input, so nothing here or in an app ever needs to know where a global sits
 * inside Absolution's prefix.
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <setjmp.h>
#include "fuzz_defs.h"
/* Optional: an app includes its own mocks.h only if it genuinely has
 * app-specific stubs. The framework symbols this harness needs are declared
 * in fuzz_defs.h. */
#if __has_include("mocks.h")
#include "mocks.h"
#endif
#include "parser.h"

extern const fuzz_command_spec_t fuzz_commands[];  ///< App command table.
extern const size_t              fuzz_n_commands;  ///< Number of entries in @ref fuzz_commands.
extern void fuzz_app_reset(void);          ///< Reset app state before each iteration (optional).
extern void fuzz_app_dispatch(void *cmd);  ///< Dispatch one @c command_t to the app (required).

/**
 * @brief Optional per-iteration teardown; the weak no-op default is in
 *        mock/fuzz_runtime.c.
 */
extern void fuzz_app_cleanup(void);

/** Lane of the input being dispatched; set before @c fuzz_app_reset() runs. */
static int fuzz_lane_structured;

/** @brief Whether control byte 0 selected the structured lane. */
static inline int fuzz_use_structured_lane(void)
{
    return fuzz_lane_structured;
}

/** @brief Clamp a raw P1/P2 byte to the command's declared maximum (0 = full range). */
static inline uint8_t fuzz_clamp_p(uint8_t raw, uint8_t p_max)
{
    if (p_max == 0) {
        return raw;
    }
    return raw % (p_max + 1);
}

/**
 * @brief Bytes an app reserves for its own header, right after the control bytes.
 *
 * The APDU payload starts after them, so @c fuzz_tail_ptr[0] stays a stable base
 * for the app's builders.
 */
#ifndef FUZZ_APP_HEADER_LEN
#define FUZZ_APP_HEADER_LEN 0
#endif

/** Apps may override these macros before inclusion to install lane-specific command tables. */
#ifndef FUZZ_PICK_COMMAND_STRUCTURED
#define FUZZ_PICK_COMMAND_STRUCTURED(data, size) (&fuzz_commands[(data)[1] % fuzz_n_commands])
#endif

#ifndef FUZZ_PICK_COMMAND_RAW
#define FUZZ_PICK_COMMAND_RAW(data, size) (&fuzz_commands[(data)[1] % fuzz_n_commands])
#endif

static void fuzz_harness_cleanup(void)
{
    fuzz_app_cleanup();
    fuzz_tail_ptr = NULL;
    fuzz_tail_len = 0;
    try_context_set(NULL);
    memset(&fuzz_exit_jump_ctx, 0, sizeof(fuzz_exit_jump_ctx));
}

/**
 * @brief Default @c fuzz_entry() body: run one fuzzer input as one APDU.
 *
 * Reads the control bytes as lane/command/P1/P2 selectors, exposes the rest as
 * the APDU payload, picks a command from @ref fuzz_commands, and dispatches it.
 * @return 0 when the input was processed, -1 when it was too short to use.
 */
static int fuzz_harness_entry(const uint8_t *data, size_t size)
{
    try_context_set(&fuzz_exit_jump_ctx);

    if (sigsetjmp(fuzz_exit_jump_ctx.jmp_buf, 1)) {
        fuzz_harness_cleanup();
        return 0;
    }

    const size_t header = (size_t) FUZZ_CTRL_LEN + FUZZ_APP_HEADER_LEN;

    if (size < header || fuzz_n_commands == 0) {
        return -1;
    }

    fuzz_lane_structured = data[0] > FUZZ_STRUCTURED_LANE_THRESHOLD;

    /* Published before fuzz_app_reset() so an app can read its own input while
     * establishing state. */
    fuzz_tail_ptr = (size > header) ? data + header : NULL;
    fuzz_tail_len = (size > header) ? size - header : 0;

    fuzz_app_reset();

    const fuzz_command_spec_t *spec;
    if (fuzz_use_structured_lane()) {
        spec = FUZZ_PICK_COMMAND_STRUCTURED(data, size);
    }
    else {
        spec = FUZZ_PICK_COMMAND_RAW(data, size);
    }

    command_t cmd;
    memset(&cmd, 0, sizeof(cmd));

    cmd.cla = spec->cla;
    cmd.ins = spec->ins;
    cmd.p1  = fuzz_clamp_p(data[2], spec->p1_max);
    cmd.p2  = fuzz_clamp_p(data[3], spec->p2_max);

    if (fuzz_tail_len > 0) {
        cmd.lc   = (uint8_t) (fuzz_tail_len > 255 ? 255 : fuzz_tail_len);
        cmd.data = (uint8_t *) fuzz_tail_ptr;
    }

    fuzz_app_dispatch(&cmd);
    fuzz_harness_cleanup();
    return 0;
}

/**
 * @brief Default custom mutator.
 *
 * Provided by the framework. A harness with its own input grammar defines
 * @c FUZZ_APP_CUSTOM_MUTATOR before including this header and provides its own
 * @c LLVMFuzzerCustomMutator(), usually built on @ref fuzz_mutate_input_with()
 * so it still does not have to know the prefix size.
 */
#include "fuzz_mutator.h"

#ifndef FUZZ_APP_CUSTOM_MUTATOR
size_t LLVMFuzzerCustomMutator(uint8_t *data, size_t size, size_t max_size, unsigned int seed)
{
    return fuzz_custom_mutator(data, size, max_size, seed);
}
#endif

/**
 * @brief Default @c fuzz_entry(), the symbol Absolution calls per iteration.
 *
 * Define @c FUZZ_APP_CUSTOM_ENTRY to write your own, for instance to add a
 * non-APDU lane. Returning -1 makes Absolution skip its @c check_invariant()
 * assertion for that input, so return it only for inputs you did not run.
 */
#ifndef FUZZ_APP_CUSTOM_ENTRY
int fuzz_entry(const uint8_t *data, size_t size);
int fuzz_entry(const uint8_t *data, size_t size)
{
    return fuzz_harness_entry(data, size);
}
#endif
