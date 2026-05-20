/*
 * Absolution dispatcher for qrcodegen (encodeBinary + encodeText).
 *
 * Tail layout: [mode_byte] [ecc_byte] [mask_byte] [payload...]
 *   mode 0 => encodeBinary (versions 1..21)
 *   mode 1 => encodeText   (versions 1..21, payload as null-terminated string)
 *
 * mask_byte bit 3 set => Mask_AUTO (-1), else mask = mask_byte % 8
 */

#include "mocks.h"
#include "parser.h"
#include "scenario_layout.h"

#include <string.h>
#include "qrcodegen.h"

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

#define MAX_VER 10

const fuzz_command_spec_t fuzz_commands[] = {
    {.cla = 0x00, .ins = 0x01},
};
const size_t fuzz_n_commands = 1;

void fuzz_app_reset(void) {}

void fuzz_app_dispatch(void *cmd)
{
    (void) cmd;
    if (!fuzz_tail_ptr || fuzz_tail_len < 3) {
        return;
    }

    uint8_t        mode_byte   = fuzz_tail_ptr[0];
    uint8_t        ecc_byte    = fuzz_tail_ptr[1];
    uint8_t        mask_byte   = fuzz_tail_ptr[2];
    const uint8_t *payload     = fuzz_tail_ptr + 3;
    size_t         payload_len = fuzz_tail_len - 3;

    enum qrcodegen_Ecc  ecc = ecc_byte % 4;
    enum qrcodegen_Mask mask
        = (mask_byte & 0x08) ? qrcodegen_Mask_AUTO : (enum qrcodegen_Mask)(mask_byte % 8);

    uint8_t dataAndTemp[qrcodegen_BUFFER_LEN_FOR_VERSION(MAX_VER)];
    uint8_t qrcode[qrcodegen_BUFFER_LEN_FOR_VERSION(MAX_VER)];

    uint8_t mode = mode_byte % 2;

    switch (mode) {
        case 0: {
            if (payload_len > sizeof(dataAndTemp)) {
                return;
            }
            memcpy(dataAndTemp, payload, payload_len);
            qrcodegen_encodeBinary(dataAndTemp, payload_len, qrcode, ecc, 1, MAX_VER, mask, true);
            break;
        }
        case 1: {
            if (payload_len == 0 || payload_len >= sizeof(dataAndTemp)) {
                return;
            }
            char text[qrcodegen_BUFFER_LEN_FOR_VERSION(MAX_VER)];
            memcpy(text, payload, payload_len);
            text[payload_len] = '\0';
            qrcodegen_encodeText(text, dataAndTemp, qrcode, ecc, 1, MAX_VER, mask, true);
            break;
        }
    }
}

int fuzz_entry(const uint8_t *data, size_t size)
{
    return fuzz_harness_entry(data, size);
}
