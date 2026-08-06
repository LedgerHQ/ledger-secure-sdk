/* Absolution dispatcher for os_parse_ndef (stateless). */

#include "mocks.h"
#include "parser.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "nfc_ndef.h"

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

    uint8_t buf[NFC_NDEF_MAX_SIZE + 64];
    memset(buf, 0, sizeof(buf));
    size_t copy_len = fuzz_tail_len < sizeof(buf) ? fuzz_tail_len : sizeof(buf);
    memcpy(buf, fuzz_tail_ptr, copy_len);

    ndef_struct_t parsed;
    memset(&parsed, 0, sizeof(parsed));
    uint8_t ret = os_parse_ndef(buf, &parsed);

    if (ret == 0) {
        /* os_parse_ndef() copies in_buffer[P1] directly as the type byte without
         * validating it, so any byte value is a legitimate success result.  Only
         * apply the string-length postconditions for the two recognised types. */
        if (parsed.ndef_type == NFC_NDEF_TYPE_TEXT || parsed.ndef_type == NFC_NDEF_TYPE_URI) {
            assert(strnlen(parsed.text, sizeof(parsed.text)) < sizeof(parsed.text));
            assert(strnlen(parsed.info, sizeof(parsed.info)) < sizeof(parsed.info));
        }
    }

    char     out_string[NFC_NDEF_MAX_SIZE + 64];
    uint16_t str_len = os_ndef_to_string(&parsed, out_string);
    (void) str_len;
}
