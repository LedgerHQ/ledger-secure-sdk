/*
 * Absolution dispatcher for tlv_use_case_dynamic_descriptor (stateless).
 *
 * Uses the TLV grammar-aware custom mutator on the tail region:
 * the Absolution prefix is handled by fuzz_mutate_with_split, then the tail
 * is mutated by tlv_custom_mutate() instead of the default LLVMFuzzerMutate.
 */

#include "mocks.h"
#include "parser.h"
#include "scenario_layout.h"

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "buffer.h"
#include "use_cases/tlv_use_case_dynamic_descriptor.h"
#include "tlv_mutator.h" /* from fuzzing/include/ */

#define FUZZ_PREFIX_SIZE_FALLBACK 0
#define FUZZ_CTRL_OFF             SCEN_CTRL_OFF
#define FUZZ_CTRL_LEN             SCEN_CTRL_LEN
#define fuzz_lane_is_structured(data, ps) \
    ((ps) > FUZZ_CTRL_OFF && (data)[FUZZ_CTRL_OFF] > FUZZ_STRUCTURED_LANE_THRESHOLD)

#include "fuzz_mutator.h"
#include "fuzz_layout_check.h"

/*
 * Custom mutator: handle the Absolution prefix with fuzz_mutate_with_split,
 * then apply TLV grammar-aware mutation to the tail.
 */
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

    /* 50% of the time: mutate prefix via the standard Absolution mutator */
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

const fuzz_command_spec_t fuzz_commands[] = {
    {.cla = 0x00, .ins = 0x01},
};
const size_t fuzz_n_commands = 1;

void fuzz_app_reset(void)
{
    current_tlv_fuzz_config.tags_info = DYNAMIC_DESCRIPTOR_TAGS;
    current_tlv_fuzz_config.num_tags
        = sizeof(DYNAMIC_DESCRIPTOR_TAGS) / sizeof(DYNAMIC_DESCRIPTOR_TAGS[0]);
}

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

int fuzz_entry(const uint8_t *data, size_t size)
{
    return fuzz_harness_entry(data, size);
}
