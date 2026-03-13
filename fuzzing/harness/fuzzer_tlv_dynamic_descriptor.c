#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "buffer.h"
#include "use_cases/tlv_use_case_dynamic_descriptor.h"
#include "tlv_mutator.h"

const tlv_tag_info_t DYNAMIC_DESCRIPTOR_TAGS_INFO[] = {
    {0x01, 1,  1 }, // TAG_STRUCTURE_TYPE : uint8, always 0x90
    {0x02, 1,  1 }, // TAG_VERSION        : uint8, must be 1
    {0x03, 4,  4 }, // TAG_COIN_TYPE      : uint32
    {0x04, 1,  33}, // TAG_APPLICATION_NAME : string, max BOLOS_APPNAME_MAX_SIZE_B+1
    {0x05, 1,  51}, // TAG_TICKER         : string, max MAX_TICKER_SIZE+1
    {0x06, 1,  1 }, // TAG_MAGNITUDE      : uint8
    {0x07, 1,  64}, // TAG_TUID           : bytes, uncapped but reasonable max
    {0x08, 70, 72}, // TAG_SIGNATURE      : DER sig
};

int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void) argc;
    (void) argv;
    // Set the grammar for the custom mutator
    current_tlv_fuzz_config.tags_info = DYNAMIC_DESCRIPTOR_TAGS_INFO;
    current_tlv_fuzz_config.num_tags
        = sizeof(DYNAMIC_DESCRIPTOR_TAGS_INFO) / sizeof(DYNAMIC_DESCRIPTOR_TAGS_INFO[0]);
    return 0;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    buffer_t                     payload = {.ptr = (uint8_t *) data, .size = size, .offset = 0};
    tlv_dynamic_descriptor_out_t out;
    memset(&out, 0, sizeof(out));

    tlv_use_case_dynamic_descriptor(&payload, &out);

    return 0;
}
