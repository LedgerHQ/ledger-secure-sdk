/*
 * Absolution dispatcher for bip32 (path_read + path_format + round-trip).
 *
 * Tail layout: [mode_byte] [path_count_byte] [payload...]
 *   mode 0 => bip32_path_read with variable out_len
 *   mode 1 => bip32_path_format with fuzz-generated path array
 *   mode 2 => read then format (round-trip)
 */

#include "mocks.h"
#include "parser.h"
#include "scenario_layout.h"

#include <string.h>
#include <assert.h>
#include "bip32.h"

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

    uint8_t        mode           = fuzz_tail_ptr[0] % 3;
    uint8_t        path_count_raw = fuzz_tail_ptr[1];
    const uint8_t *payload        = fuzz_tail_ptr + 2;
    size_t         payload_len    = fuzz_tail_len - 2;

    /* Allow edge cases: 0 and values > MAX_BIP32_PATH */
    size_t path_count = path_count_raw % 12;

    switch (mode) {
        case 0: {
            uint32_t out[MAX_BIP32_PATH];
            bip32_path_read(payload, payload_len, out, path_count);
            break;
        }
        case 1: {
            if (path_count == 0 || path_count > MAX_BIP32_PATH) {
                break;
            }
            if (payload_len < path_count * 4) {
                break;
            }

            uint32_t path[MAX_BIP32_PATH];
            for (size_t i = 0; i < path_count; i++) {
                path[i] = ((uint32_t) payload[i * 4] << 24) | ((uint32_t) payload[i * 4 + 1] << 16)
                          | ((uint32_t) payload[i * 4 + 2] << 8) | (uint32_t) payload[i * 4 + 3];
            }

            size_t out_len = 150;
            if (payload_len > path_count * 4) {
                out_len = payload[path_count * 4] + 1;
            }
            char out[200];
            if (out_len > sizeof(out)) {
                out_len = sizeof(out);
            }
            bip32_path_format(path, path_count, out, out_len);
            break;
        }
        case 2: {
            if (path_count == 0 || path_count > MAX_BIP32_PATH) {
                break;
            }
            uint32_t out[MAX_BIP32_PATH];
            if (!bip32_path_read(payload, payload_len, out, path_count)) {
                break;
            }

            char formatted[200];
            bip32_path_format(out, path_count, formatted, sizeof(formatted));
            break;
        }
    }
}

int fuzz_entry(const uint8_t *data, size_t size)
{
    return fuzz_harness_entry(data, size);
}
