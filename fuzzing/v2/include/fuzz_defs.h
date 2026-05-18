#pragma once
/* Shared constants and types for SDK fuzz harnesses. */

#include <stdint.h>

#define FUZZ_STRUCTURED_LANE_THRESHOLD 102 /* byte 0: > threshold => structured lane */

#define FUZZ_CMD_HAS_DATA (1u << 0) /* command spec carries an APDU payload */

/* Shared command spec type for app-defined command tables.
 * FUZZ_PICK_COMMAND_* selects entries from tables of this type. */
typedef struct {
    uint8_t cla;
    uint8_t ins;
    uint8_t p1_max; /* 0 = full range [0,255] */
    uint8_t p2_max; /* 0 = full range [0,255] */
    uint8_t flags;  /* FUZZ_CMD_* bitfield */
} fuzz_command_spec_t;
