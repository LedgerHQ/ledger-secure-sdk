#include "mocks.h"
#include <stdint.h>

/* TODO: include the app's headers (globals.h, dispatcher.h, types.h, ...). */

#include "scenario_layout.h"

#define FUZZ_PREFIX_SIZE_FALLBACK SCEN_PREFIX_SIZE
#define FUZZ_CTRL_OFF             SCEN_CTRL_OFF
#define FUZZ_CTRL_LEN             SCEN_CTRL_LEN
#define fuzz_lane_is_structured(data, ps) \
    ((ps) > FUZZ_CTRL_OFF && (data)[FUZZ_CTRL_OFF] > FUZZ_STRUCTURED_LANE_THRESHOLD)

#include "fuzz_mutator.h"
#include "fuzz_layout_check.h"

size_t LLVMFuzzerCustomMutator(uint8_t *data, size_t size, size_t max_size, unsigned int seed)
{
    return fuzz_custom_mutator(data, size, max_size, seed);
}

#define CLA_APP 0xE0 /* TODO: replace with the app's CLA. */

#include "fuzz_harness.h"

/* TODO: replace with the dispatcher's accepted commands.
 * Set .p1_max / .p2_max when only part of the byte range is valid. */
const fuzz_command_spec_t fuzz_commands[] = {
    {.cla = CLA_APP, .ins = 0x00},
    {.cla = CLA_APP, .ins = 0x01, .p1_max = 1, .flags = FUZZ_CMD_HAS_DATA},
};

const size_t fuzz_n_commands = sizeof(fuzz_commands) / sizeof(fuzz_commands[0]);

void fuzz_app_reset(void)
{
    /* TODO: clear persistent app globals before each iteration. */
}

void fuzz_app_dispatch(void *cmd)
{
    /* TODO: call the real APDU dispatcher.
     * Example: apdu_dispatcher((const command_t *)cmd); */
    (void) cmd;
}

int fuzz_entry(const uint8_t *data, size_t size)
{
    return fuzz_harness_entry(data, size);
}
