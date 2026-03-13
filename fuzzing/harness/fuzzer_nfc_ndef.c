#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "nfc_ndef.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    ndef_struct_t parsed;
    char          out_string[NFC_NDEF_MAX_SIZE + 1];
    uint8_t      *in_buffer;

    if (size == 0) {
        return 0;
    }

    in_buffer = (uint8_t *) malloc(size);
    if (in_buffer == NULL) {
        return 0;
    }

    memset(&parsed, 0, sizeof(parsed));
    memset(out_string, 0, sizeof(out_string));
    memcpy(in_buffer, data, size);

    os_parse_ndef(in_buffer, &parsed);
    os_ndef_to_string(&parsed, out_string);
    free(in_buffer);

    return 0;
}