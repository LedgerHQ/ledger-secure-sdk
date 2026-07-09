#pragma once
/**
 * @file fuzz_defs.h
 * @brief Shared constants and the command descriptor type for SDK fuzz harnesses.
 */

#include <stdint.h>

/**
 * @brief Control byte 0 above this value selects the structured lane; at or
 *        below it the harness takes the raw lane.
 *
 * Structured lane (@c fuzz_ctrl[0] > 102): harness picks a command from the
 * table via @c fuzz_ctrl[1], clamps P1/P2, and builds a valid APDU — drives
 * the happy path.  Raw lane (≤ 102): all APDU fields come straight from the
 * tail bytes without clamping — exercises parsing edges and error paths.
 *
 * The value 102 gives a ≈40/60 raw/structured split across 0–255; apps do not
 * tune it.  Most apps define one @c fuzz_commands[] and let both lanes share
 * it.  Override @c FUZZ_PICK_COMMAND_STRUCTURED / @c FUZZ_PICK_COMMAND_RAW
 * only when two structurally different entry paths need separate tables.
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
