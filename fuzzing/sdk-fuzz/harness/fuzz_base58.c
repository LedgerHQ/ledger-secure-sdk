/*
 * Absolution dispatcher for base58 (decode + encode + round-trip).
 *
 * Byte 0 of the tail selects the mode:
 *   0 => decode only
 *   1 => encode only
 *   2 => encode then decode (round-trip with assertion)
 */

#include "mocks.h"
#include "parser.h"
#include "scenario_layout.h"

#include <string.h>
#include <assert.h>
#include "base58.h"

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

#include "fuzz_harness.h"

const fuzz_command_spec_t fuzz_commands[] = {
    {.cla = 0x00, .ins = 0x01},
    {.cla = 0x00, .ins = 0x02},
    {.cla = 0x00, .ins = 0x03},
};
const size_t fuzz_n_commands = 3;

void fuzz_app_reset(void) {}

void fuzz_app_dispatch(void *cmd)
{
    (void) cmd;
    if (!fuzz_tail_ptr || fuzz_tail_len < 2) {
        return;
    }

    uint8_t        mode        = fuzz_tail_ptr[0] % 3;
    const uint8_t *payload     = fuzz_tail_ptr + 1;
    size_t         payload_len = fuzz_tail_len - 1;

    switch (mode) {
        case 0: {
            uint8_t out[200];
            base58_decode((const char *) payload, payload_len, out, sizeof(out));
            break;
        }
        case 1: {
            char out[200];
            base58_encode(payload, payload_len, out, sizeof(out));
            break;
        }
        case 2: {
            if (payload_len == 0 || payload_len > 120) {
                break;
            }
            char encoded[200];
            int  enc_len = base58_encode(payload, payload_len, encoded, sizeof(encoded));
            if (enc_len <= 0) {
                break;
            }

            uint8_t decoded[200];
            int     dec_len = base58_decode(encoded, (size_t) enc_len, decoded, sizeof(decoded));
            if (dec_len > 0) {
                assert((size_t) dec_len == payload_len);
                assert(memcmp(decoded, payload, payload_len) == 0);
            }
            break;
        }
    }
}

int fuzz_entry(const uint8_t *data, size_t size)
{
    return fuzz_harness_entry(data, size);
}
