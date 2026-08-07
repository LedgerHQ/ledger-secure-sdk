/* Absolution dispatcher for qrcodegen (encodeBinary/encodeText). */

#include "mocks.h"
#include "parser.h"

#include <string.h>
#include "qrcodegen.h"

#include "fuzz_harness.h"

#define MAX_VER 10

const fuzz_command_spec_t fuzz_commands[] = {
    {.cla = 0x00, .ins = 0x01},
};
FUZZ_COMMAND_COUNT();

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
            qrcodegen_encodeBinary(
                dataAndTemp, payload_len, qrcode, ecc, qrcodegen_VERSION_MIN, MAX_VER, mask, true);
            break;
        }
        case 1: {
            if (payload_len == 0 || payload_len >= sizeof(dataAndTemp)) {
                return;
            }
            char text[qrcodegen_BUFFER_LEN_FOR_VERSION(MAX_VER)];
            memcpy(text, payload, payload_len);
            text[payload_len] = '\0';
            qrcodegen_encodeText(
                text, dataAndTemp, qrcode, ecc, qrcodegen_VERSION_MIN, MAX_VER, mask, true);
            break;
        }
    }
}
