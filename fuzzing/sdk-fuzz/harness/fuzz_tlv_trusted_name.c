/*
 * Absolution dispatcher for tlv_use_case_trusted_name (stateless).
 *
 * Uses the TLV grammar-aware custom mutator on the tail region.
 */

#include "mocks.h"
#include "parser.h"
#include "scenario_layout.h"

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "buffer.h"
#include "use_cases/tlv_use_case_trusted_name.h"
#include "tlv_mutator.h" /* from fuzzing/include/ */

#define FUZZ_PREFIX_SIZE_FALLBACK 0
#define FUZZ_CTRL_OFF             SCEN_CTRL_OFF
#define FUZZ_CTRL_LEN             SCEN_CTRL_LEN
#define fuzz_lane_is_structured(data, ps) \
    ((ps) > FUZZ_CTRL_OFF && (data)[FUZZ_CTRL_OFF] > FUZZ_STRUCTURED_LANE_THRESHOLD)

#include "fuzz_mutator.h"
#include "fuzz_layout_check.h"

extern const size_t absolution_globals_size __attribute__((weak));

size_t LLVMFuzzerCustomMutator(uint8_t *data, size_t size, size_t max_size, unsigned int seed)
{
    size_t prefix_size = FUZZ_PREFIX_SIZE_FALLBACK;
    if (&absolution_globals_size != NULL && absolution_globals_size != 0) {
        prefix_size = absolution_globals_size;
    }

    if (prefix_size == 0 || prefix_size >= max_size || size <= prefix_size) {
        return fuzz_custom_mutator(data, size, max_size, seed);
    }

    if ((seed & 1U) == 0) {
        size_t tail_size = size - prefix_size;
        tail_size
            = tlv_custom_mutate(data + prefix_size, tail_size, max_size - prefix_size, seed >> 1);
        return prefix_size + tail_size;
    }

    return fuzz_custom_mutator(data, size, max_size, seed);
}

#include "fuzz_harness.h"

/* ── TLV grammar ─────────────────────────────────────────────────────────── */

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

const fuzz_command_spec_t fuzz_commands[] = {
    {.cla = 0x00, .ins = 0x01},
};
const size_t fuzz_n_commands = 1;

void fuzz_app_reset(void)
{
    current_tlv_fuzz_config.tags_info = TRUSTED_NAME_TAGS;
    current_tlv_fuzz_config.num_tags  = sizeof(TRUSTED_NAME_TAGS) / sizeof(TRUSTED_NAME_TAGS[0]);
}

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

int fuzz_entry(const uint8_t *data, size_t size)
{
    return fuzz_harness_entry(data, size);
}
