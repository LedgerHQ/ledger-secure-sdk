#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "buffer.h"
#include "use_cases/tlv_use_case_trusted_name.h"
#include "tlv_mutator.h"

// Grammar for the Trusted Name TLV use case: {Tag, Min length, Max length}
const tlv_tag_info_t TRUSTED_NAME_TAGS_INFO[] = {
    {0x01, 1,  1 }, // TAG_STRUCTURE_TYPE (uint8_t)
    {0x02, 1,  1 }, // TAG_VERSION (uint8_t)
    {0x70, 1,  1 }, // TAG_TRUSTED_NAME_TYPE (uint8_t)
    {0x71, 1,  1 }, // TAG_TRUSTED_NAME_SOURCE (uint8_t)
    {0x20, 1,  64}, // TAG_TRUSTED_NAME (String, max TRUSTED_NAME_STRINGS_MAX_SIZE)
    {0x23, 1,  8 }, // TAG_CHAIN_ID (uint64_t, 1-8 bytes DER)
    {0x22, 1,  40}, // TAG_ADDRESS (Buffer)
    {0x72, 32, 32}, // TAG_NFT_ID (32 bytes fixed)
    {0x73, 1,  40}, // TAG_SOURCE_CONTRACT (Buffer)
    {0x12, 1,  4 }, // TAG_CHALLENGE (uint32_t, 1-4 bytes DER)
    {0x10, 4,  4 }, // TAG_NOT_VALID_AFTER (semver: major + minor + patch = 4 bytes)
    {0x13, 1,  2 }, // TAG_SIGNER_KEY_ID (uint16_t, 1-2 bytes DER)
    {0x14, 1,  1 }, // TAG_SIGNER_ALGORITHM (uint8_t)
    {0x15, 64, 72}, // TAG_DER_SIGNATURE (ECDSA DER 64-72 bytes)
};

int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void) argc;
    (void) argv;
    // Set the grammar for the custom mutator
    current_tlv_fuzz_config.tags_info = TRUSTED_NAME_TAGS_INFO;
    current_tlv_fuzz_config.num_tags
        = sizeof(TRUSTED_NAME_TAGS_INFO) / sizeof(TRUSTED_NAME_TAGS_INFO[0]);
    return 0;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    buffer_t               payload = {.ptr = (uint8_t *) data, .size = size, .offset = 0};
    tlv_trusted_name_out_t out;
    memset(&out, 0, sizeof(out));

    tlv_use_case_trusted_name(&payload, &out);

    return 0;
}
