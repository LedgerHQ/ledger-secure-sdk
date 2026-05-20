/*
 * Absolution dispatcher for os_parse_ndef (stateless).
 *
 * Exercises the NDEF parser from lib_nfc with arbitrary payloads.
 */

#include "mocks.h"
#include "parser.h"
#include "scenario_layout.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "nfc_ndef.h"

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
};
const size_t fuzz_n_commands = 1;

void fuzz_app_reset(void) {}

void fuzz_app_dispatch(void *cmd)
{
    (void) cmd;
    if (!fuzz_tail_ptr || fuzz_tail_len == 0) {
        return;
    }

    uint8_t buf[NFC_NDEF_MAX_SIZE + 64];
    size_t  copy_len = fuzz_tail_len < sizeof(buf) ? fuzz_tail_len : sizeof(buf);
    memcpy(buf, fuzz_tail_ptr, copy_len);

    ndef_struct_t parsed;
    memset(&parsed, 0, sizeof(parsed));
    uint8_t ret = os_parse_ndef(buf, &parsed);

    if (ret == 0) {
        assert(parsed.ndef_type == NFC_NDEF_TYPE_TEXT || parsed.ndef_type == NFC_NDEF_TYPE_URI);
        assert(strnlen(parsed.text, sizeof(parsed.text)) < sizeof(parsed.text));
        assert(strnlen(parsed.info, sizeof(parsed.info)) < sizeof(parsed.info));
    }

    char     out_string[NFC_NDEF_MAX_SIZE + 64];
    uint16_t str_len = os_ndef_to_string(&parsed, out_string);
    (void) str_len;
}

int fuzz_entry(const uint8_t *data, size_t size)
{
    return fuzz_harness_entry(data, size);
}
