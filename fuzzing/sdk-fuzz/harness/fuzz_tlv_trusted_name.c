/* Absolution dispatcher for tlv_use_case_trusted_name (stateless). */

#include "mocks.h"
#include "parser.h"

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "buffer.h"
#include "use_cases/tlv_use_case_trusted_name.h"
#include "tlv_mutator.h"

/* This target has its own TLV grammar, so it supplies its own mutator. */
#define FUZZ_APP_CUSTOM_MUTATOR
#include "fuzz_harness.h"

static const tlv_tag_info_t TRUSTED_NAME_TAGS[] = {
    {0x01, 1,  1 }, /* TAG_STRUCTURE_TYPE */
    {0x02, 1,  1 }, /* TAG_VERSION */
    {0x70, 1,  1 }, /* TAG_TRUSTED_NAME_TYPE */
    {0x71, 1,  1 }, /* TAG_TRUSTED_NAME_SOURCE */
    {0x20, 1,  64}, /* TAG_TRUSTED_NAME */
    {0x23, 1,  8 }, /* TAG_CHAIN_ID */
    {0x22, 1,  40}, /* TAG_ADDRESS */
    {0x72, 32, 32}, /* TAG_NFT_ID */
    {0x73, 1,  40}, /* TAG_SOURCE_CONTRACT */
    {0x12, 1,  4 }, /* TAG_CHALLENGE */
    {0x10, 4,  4 }, /* TAG_NOT_VALID_AFTER */
    {0x13, 1,  2 }, /* TAG_SIGNER_KEY_ID */
    {0x14, 1,  1 }, /* TAG_SIGNER_ALGORITHM */
    {0x15, 64, 72}, /* TAG_DER_SIGNATURE */
};

size_t LLVMFuzzerCustomMutator(uint8_t *data, size_t size, size_t max_size, unsigned int seed)
{
    if ((seed & 1U) == 0) {
        /* Install the grammar for the mutator only. current_tlv_fuzz_config is a
         * zero-symbol the invariant enforces on the test path, so it must never
         * be left set there; sample_invariant() re-zeroes it before dispatch. */
        current_tlv_fuzz_config.tags_info = TRUSTED_NAME_TAGS;
        current_tlv_fuzz_config.num_tags = sizeof(TRUSTED_NAME_TAGS) / sizeof(TRUSTED_NAME_TAGS[0]);
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
    tlv_trusted_name_out_t out;
    memset(&out, 0, sizeof(out));

    tlv_use_case_trusted_name(&payload, &out);
}
