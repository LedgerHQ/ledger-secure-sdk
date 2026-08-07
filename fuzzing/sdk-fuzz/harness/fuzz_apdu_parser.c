/* Absolution dispatcher for apdu_parser (stateless). */

#include "mocks.h"
#include "parser.h"

#include <assert.h>
#include <string.h>

#define IO_APDU_BUFFER_SIZE (5 + 255)

#include "fuzz_harness.h"

const fuzz_command_spec_t fuzz_commands[] = {
    {.cla = 0x00, .ins = 0x01},
};
FUZZ_COMMAND_COUNT();

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
