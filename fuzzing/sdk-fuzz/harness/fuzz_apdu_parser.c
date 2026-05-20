/*
 * Absolution dispatcher for apdu_parser (stateless).
 */

#include "mocks.h"
#include "parser.h"
#include "scenario_layout.h"

#include <assert.h>
#include <string.h>

#define FUZZ_PREFIX_SIZE_FALLBACK 0
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

#define IO_APDU_BUFFER_SIZE (5 + 255)

#include "fuzz_harness.h"

const fuzz_command_spec_t fuzz_commands[] = {
    {.cla = 0x00, .ins = 0x01},
};
const size_t fuzz_n_commands = 1;

void fuzz_app_reset(void) {}

void fuzz_app_dispatch(void *cmd)
{
    (void) cmd;
    if (!fuzz_tail_ptr || fuzz_tail_len == 0) {
        return;
    }
    if (fuzz_tail_len > IO_APDU_BUFFER_SIZE) {
        return;
    }

    uint8_t   apdu_message[IO_APDU_BUFFER_SIZE];
    command_t parsed;
    memset(&parsed, 0, sizeof(parsed));
    memcpy(apdu_message, fuzz_tail_ptr, fuzz_tail_len);

    if (apdu_parser(&parsed, apdu_message, fuzz_tail_len)) {
        if (parsed.lc == 0) {
            assert(parsed.data == NULL);
        }
        else {
            assert(parsed.data == apdu_message + 5);
            assert(fuzz_tail_len >= 5 + (size_t) parsed.lc);
        }
    }
}

int fuzz_entry(const uint8_t *data, size_t size)
{
    return fuzz_harness_entry(data, size);
}
