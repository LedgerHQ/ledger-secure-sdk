/* Absolution dispatcher for tlv_use_case_dynamic_descriptor (stateless). */

#include "mocks.h"
#include "parser.h"

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "buffer.h"
#include "use_cases/tlv_use_case_dynamic_descriptor.h"
#include "tlv_mutator.h"

/* This target has its own TLV grammar, so it supplies its own mutator. */
#define FUZZ_APP_CUSTOM_MUTATOR
#include "fuzz_harness.h"

static const tlv_tag_info_t DYNAMIC_DESCRIPTOR_TAGS[] = {
    {0x01, 1,  1 }, /* TAG_STRUCTURE_TYPE */
    {0x02, 1,  1 }, /* TAG_VERSION */
    {0x03, 4,  4 }, /* TAG_COIN_TYPE */
    {0x04, 1,  33}, /* TAG_APPLICATION_NAME */
    {0x05, 1,  51}, /* TAG_TICKER */
    {0x06, 1,  1 }, /* TAG_MAGNITUDE */
    {0x07, 1,  64}, /* TAG_TUID */
    {0x08, 70, 72}, /* TAG_SIGNATURE */
};

size_t LLVMFuzzerCustomMutator(uint8_t *data, size_t size, size_t max_size, unsigned int seed)
{
    if ((seed & 1U) == 0) {
        /* Install the grammar for the mutator only. current_tlv_fuzz_config is a
         * zero-symbol the invariant enforces on the test path, so it must never
         * be left set there; sample_invariant() re-zeroes it before dispatch. */
        current_tlv_fuzz_config.tags_info = DYNAMIC_DESCRIPTOR_TAGS;
        current_tlv_fuzz_config.num_tags
            = sizeof(DYNAMIC_DESCRIPTOR_TAGS) / sizeof(DYNAMIC_DESCRIPTOR_TAGS[0]);
        return fuzz_mutate_input_with(data, size, max_size, seed >> 1, tlv_custom_mutate);
    }

    return fuzz_custom_mutator(data, size, max_size, seed);
}

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

    buffer_t payload = {.ptr = (uint8_t *) fuzz_tail_ptr, .size = fuzz_tail_len, .offset = 0};
    tlv_dynamic_descriptor_out_t out;
    memset(&out, 0, sizeof(out));

    tlv_use_case_dynamic_descriptor(&payload, &out);
}
