/* Absolution dispatcher for base58 (decode/encode/round-trip). */

#include "mocks.h"
#include "parser.h"

#include <string.h>
#include <assert.h>
#include "base58.h"

#include "fuzz_harness.h"

const fuzz_command_spec_t fuzz_commands[] = {
    {.cla = 0x00, .ins = 0x01},
    {.cla = 0x00, .ins = 0x02},
    {.cla = 0x00, .ins = 0x03},
};
FUZZ_COMMAND_COUNT();

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
