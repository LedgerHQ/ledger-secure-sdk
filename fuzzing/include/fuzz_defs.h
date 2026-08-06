#pragma once
/**
 * @file fuzz_defs.h
 * @brief Shared constants, the input layout, and the command descriptor type for
 *        SDK fuzz harnesses.
 *
 * ## Input layout
 *
 * A fuzzer input is Absolution's sampled global state followed by the part the
 * harness sees:
 *
 *     [ Absolution prefix: FUZZ_PREFIX_SIZE bytes ][ harness input ]
 *                     opaque sampled state            layout below
 *
 * @c sample_invariant() consumes the prefix and passes the rest to
 * @c fuzz_entry(), so every offset a harness or the mutator cares about is
 * relative to the start of the harness input.
 *
 * | byte    | meaning                                                  |
 * | ------- | -------------------------------------------------------- |
 * | 0       | lane selector (> @ref FUZZ_STRUCTURED_LANE_THRESHOLD)    |
 * | 1       | command index (taken modulo the app's command count)      |
 * | 2       | P1 (clamped to the command's @c p1_max)                   |
 * | 3       | P2 (clamped to the command's @c p2_max)                   |
 * | 4..     | app header, @c FUZZ_APP_HEADER_LEN bytes (0 by default)   |
 * | then    | APDU payload, exposed to the app as @c fuzz_tail_ptr/len  |
 *
 * An app that needs its own control or entropy bytes defines
 * @c FUZZ_APP_HEADER_LEN and reads them at @c data[FUZZ_CTRL_LEN]. The payload
 * then starts after them, so builders keep a stable base at
 * @c fuzz_tail_ptr[0].
 */

#include <stddef.h>
#include <stdint.h>

#include "exceptions.h" /* try_context_t */

/**
 * @name Framework-owned runtime symbols
 *
 * Declared here and defined in mock/fuzz_runtime.c, which every target links
 * through the shared @c mock library. The framework writes them; an app never
 * declares or defines them.
 * @{
 */

/** @brief The APDU payload for the current iteration (NULL when empty). */
extern const uint8_t *fuzz_tail_ptr;

/** @brief Length of @ref fuzz_tail_ptr in bytes. */
extern size_t fuzz_tail_len;

/**
 * @brief Size in bytes of Absolution's sampled state prefix.
 *
 * Defined in a one-line translation unit the build generates from the generated
 * fuzzer (see cmake/EmitPrefixSize.cmake).
 */
extern const size_t fuzz_absolution_prefix_size;

/**
 * @brief longjmp landing pad the harness unwinds os_exit() through.
 *
 * Defined in mock/os/os_exceptions.c. Declared here because fuzz_harness.h uses
 * it, so an app's mock/mocks.h does not need to.
 */
extern try_context_t fuzz_exit_jump_ctx;

/** @} */

/** @brief Bytes 0..3 steer the harness; byte 4 onwards is payload. */
#define FUZZ_CTRL_LEN 4

/**
 * @brief Control byte 0 above this value selects the structured lane; at or
 *        below it the harness takes the raw lane.
 *
 * Structured lane (@c data[0] > 102): the harness picks a command from the table
 * via @c data[1], clamps P1/P2, and builds a valid APDU — drives the happy path.
 * Raw lane (≤ 102): all APDU fields come straight from the input bytes without
 * clamping — exercises parsing edges and error paths.
 *
 * The value 102 gives a ≈40/60 raw/structured split across 0–255; apps do not
 * tune it. Most apps define one @c fuzz_commands[] and let both lanes share it.
 * Override @c FUZZ_PICK_COMMAND_STRUCTURED / @c FUZZ_PICK_COMMAND_RAW only when
 * two structurally different entry paths need separate tables.
 */
#define FUZZ_STRUCTURED_LANE_THRESHOLD 102

/** @ref fuzz_command_spec_t flag: the command carries an APDU payload. */
#define FUZZ_CMD_HAS_DATA (1u << 0)

/** @brief Describes one APDU command the harness may synthesise and dispatch. */
typedef struct {
    uint8_t cla;     ///< APDU class byte.
    uint8_t ins;     ///< APDU instruction byte.
    uint8_t p1_max;  ///< Upper bound for P1 (0 = full range [0,255]).
    uint8_t p2_max;  ///< Upper bound for P2 (0 = full range [0,255]).
    uint8_t flags;   ///< Bitfield of FUZZ_CMD_* flags.
} fuzz_command_spec_t;

/** @brief Derive @c fuzz_n_commands from the app's @c fuzz_commands[] table. */
#define FUZZ_COMMAND_COUNT() \
    const size_t fuzz_n_commands = sizeof(fuzz_commands) / sizeof(fuzz_commands[0])
