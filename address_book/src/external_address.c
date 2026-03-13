/*****************************************************************************
 *   (c) 2026 Ledger SAS.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *****************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "lcx_ecdsa.h"
#include "lcx_sha256.h"
#include "os_utils.h"
#include "os_apdu.h"
#include "os_pki.h"
#include "ledger_pki.h"
#include "status_words.h"
#include "tlv_library.h"
#include "buffer.h"
#include "address_book.h"
#include "address_book_entrypoints.h"
#include "external_address.h"

#if defined(HAVE_ADDRESS_BOOK)

/* Private defines------------------------------------------------------------*/
#define TYPE_EXTERNAL_ADDRESS 0x11
#define STRUCT_VERSION        0x01

/* Private enumerations ------------------------------------------------------*/

/* Private types, structures, unions -----------------------------------------*/
typedef struct {
    external_address_t *external_address;
    uint8_t             sig_size;
    const uint8_t      *sig;
    cx_sha256_t         hash_ctx;
    TLV_reception_t     received_tags;
} s_ext_addr_ctx;

/* Private macros-------------------------------------------------------------*/
// Define TLV tags for External address
#define EXTERNAL_ADDRESS_TAGS(X)                                                 \
    X(0x01, TAG_STRUCTURE_TYPE, handle_struct_type, ENFORCE_UNIQUE_TAG)          \
    X(0x02, TAG_STRUCTURE_VERSION, handle_struct_version, ENFORCE_UNIQUE_TAG)    \
    X(0x0A, TAG_CONTACT_NAME, handle_contact_name, ENFORCE_UNIQUE_TAG)           \
    X(0x0B, TAG_ADDRESS_NAME, handle_address_name, ENFORCE_UNIQUE_TAG)           \
    X(0x22, TAG_ADDRESS_RAW, handle_address_raw, ENFORCE_UNIQUE_TAG)             \
    X(0x23, TAG_CHAIN_ID, handle_chain_id, ENFORCE_UNIQUE_TAG)                   \
    X(0x51, TAG_BLOCKCHAIN_FAMILY, handle_blockchain_family, ENFORCE_UNIQUE_TAG) \
    X(0x15, TAG_DER_SIGNATURE, handle_signature, ENFORCE_UNIQUE_TAG)

/* Private functions prototypes ----------------------------------------------*/
static bool common_handler(const tlv_data_t *data, s_ext_addr_ctx *context);

/* Exported variables --------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
// Global structure to store the External Address
static external_address_t EXT_ADDR = {0};

/* Private functions ---------------------------------------------------------*/

/**
 * @brief Handler for tag \ref STRUCTURE_TYPE.
 *
 * @param[in] data the tlv data
 * @param[in] context the received payload
 * @return whether the handling was successful
 */
static bool handle_struct_type(const tlv_data_t *data, s_ext_addr_ctx *context)
{
    UNUSED(context);
    return tlv_check_structure_type(data, TYPE_EXTERNAL_ADDRESS);
}

/**
 * @brief Handler for tag \ref STRUCTURE_VERSION.
 *
 * @param[in] data the tlv data
 * @param[in] context the received payload
 * @return whether the handling was successful
 */
static bool handle_struct_version(const tlv_data_t *data, s_ext_addr_ctx *context)
{
    UNUSED(context);
    return tlv_check_structure_version(data, STRUCT_VERSION);
}

/**
 * @brief Handler for tag \ref CONTACT_NAME.
 *
 * @param[in] data the tlv data
 * @param[in] context the received payload
 * @return whether the handling was successful
 */
static bool handle_contact_name(const tlv_data_t *data, s_ext_addr_ctx *context)
{
    // Extract the string (with null terminator added by get_string_from_tlv_data)
    if (!get_string_from_tlv_data(data,
                                  (char *) context->external_address->contact_name,
                                  1,
                                  sizeof(context->external_address->contact_name) - 1)) {
        PRINTF("CONTACT_NAME: failed to extract\n");
        return false;
    }
    if (!is_printable_string((const char *) context->external_address->contact_name,
                             strlen((const char *) context->external_address->contact_name))) {
        PRINTF("CONTACT_NAME: error\n");
        return false;
    }
    return true;
}

/**
 * @brief Handler for tag \ref ADDRESS_NAME.
 *
 * @param[in] data the tlv data
 * @param[in] context the received payload
 * @return whether the handling was successful
 */
static bool handle_address_name(const tlv_data_t *data, s_ext_addr_ctx *context)
{
    // Extract the string (with null terminator added by get_string_from_tlv_data)
    if (!get_string_from_tlv_data(data,
                                  (char *) context->external_address->address_name,
                                  1,
                                  sizeof(context->external_address->address_name) - 1)) {
        PRINTF("ADDRESS_NAME: failed to extract\n");
        return false;
    }
    if (!is_printable_string((const char *) context->external_address->address_name,
                             strlen((const char *) context->external_address->address_name))) {
        PRINTF("ADDRESS_NAME: error\n");
        return false;
    }
    return true;
}

/**
 * @brief Handler for tag \ref ADDRESS_RAW.
 *
 * @param[in] data the tlv data
 * @param[in] context the received payload
 * @return whether the handling was successful
 */
static bool handle_address_raw(const tlv_data_t *data, s_ext_addr_ctx *context)
{
    buffer_t address = {0};
    if (!get_buffer_from_tlv_data(data, &address, 1, MAX_ADDRESS_LENGTH)) {
        PRINTF("ADDRESS_RAW: failed to extract\n");
        return false;
    }
    if (is_zeroes_buffer(address.ptr, address.size) == true) {
        PRINTF("ADDRESS_RAW: all zeroes\n");
        return false;
    }
    memmove((void *) context->external_address->address_raw, (void *) address.ptr, address.size);
    context->external_address->address_length = address.size;
    return true;
}

/**
 * @brief Parse the CHAIN_ID value.
 *
 * @param[in] data data to handle
 * @param[out] context the received payload
 * @return whether the handling was successful
 */
static bool handle_chain_id(const tlv_data_t *data, s_ext_addr_ctx *context)
{
    if (!get_uint64_t_from_tlv_data(data, &context->external_address->chain_id)) {
        PRINTF("CHAIN_ID: failed to extract\n");
        return false;
    }
    return true;
}

/**
 * @brief Parse the BLOCKCHAIN_FAMILY value.
 *
 * @param[in] data data to handle
 * @param[out] context the received payload
 * @return whether the handling was successful
 */
static bool handle_blockchain_family(const tlv_data_t *data, s_ext_addr_ctx *context)
{
    UNUSED(context);
    uint8_t value = 0;
    if (!get_uint8_t_from_tlv_data(data, &value)) {
        PRINTF("UINT8: failed to extract\n");
        return false;
    }
    if (value >= FAMILY_MAX) {
        PRINTF("BLOCKCHAIN_FAMILY: Value out of range (%d)!\n", FAMILY_MAX);
        return false;
    }
    context->external_address->blockchain_family = (blockchain_family_e) value;
    return true;
}

/**
 * Handler for tag \ref DER_SIGNATURE
 *
 * @param[in] data the tlv data
 * @param[in] context the received payload
 * @return whether the handling was successful
 */
static bool handle_signature(const tlv_data_t *data, s_ext_addr_ctx *context)
{
    buffer_t sig = {0};
    if (!get_buffer_from_tlv_data(
            data, &sig, CX_ECDSA_SHA256_SIG_MIN_ASN1_LENGTH, CX_ECDSA_SHA256_SIG_MAX_ASN1_LENGTH)) {
        PRINTF("DER_SIGNATURE: failed to extract\n");
        return false;
    }
    context->sig_size = sig.size;
    context->sig      = sig.ptr;
    return true;
}

/**
 * @brief Define the TLV parser for External Address.
 *
 * This macro generates a TLV parser function named ext_addr_tlv_parser that processes
 * External Address payloads according to the EXTERNAL_ADDRESS_TAGS specification.
 * The parser iterates through TLV-encoded data, invokes specific tag handlers, and
 * calls the common_handler for additional processing (such as hashing).
 */
DEFINE_TLV_PARSER(EXTERNAL_ADDRESS_TAGS, &common_handler, ext_addr_tlv_parser)

/**
 * @brief Common handler that hashes all tags except signature
 *
 * @param[in] data the tlv data
 * @param[in] context the received payload
 * @return true on success
 */
static bool common_handler(const tlv_data_t *data, s_ext_addr_ctx *context)
{
    // Hash all tags except the signature
    if (data->tag != TAG_DER_SIGNATURE) {
        CX_ASSERT(cx_hash_no_throw(
            (cx_hash_t *) &context->hash_ctx, 0, data->raw.ptr, data->raw.size, NULL, 0));
    }
    return true;
}

/**
 * @brief Verify the payload signature
 *
 * Verify the SHA-256 hash of the payload against the public key
 *
 * @param[in] context the received payload
 * @return whether it was successful
 */
static bool verify_signature(const s_ext_addr_ctx *context)
{
    uint8_t          hash[CX_SHA256_SIZE] = {0};
    const buffer_t   buffer               = {.ptr = hash, .size = sizeof(hash)};
    const buffer_t   signature = {.ptr = (uint8_t *) context->sig, .size = context->sig_size};
    const cx_curve_t curve     = CX_CURVE_256K1;
    const uint8_t    key_usage = CERTIFICATE_PUBLIC_KEY_USAGE_ADDRESS_BOOK;

    if (cx_hash_no_throw((cx_hash_t *) &context->hash_ctx, CX_LAST, NULL, 0, hash, sizeof(hash))
        != CX_OK) {
        PRINTF("Could not finalize struct hash!\n");
        return false;
    }

    if (check_signature_with_pki(buffer, &key_usage, &curve, signature)
        != CHECK_SIGNATURE_WITH_PKI_SUCCESS) {
        return false;
    }

    return true;
}

/**
 * @brief Verify the received fields
 *
 * Check the mandatory fields are present
 *
 * @param[in] context the received payload
 * @return whether it was successful
 */
static bool verify_fields(const s_ext_addr_ctx *context)
{
    bool result = TLV_CHECK_RECEIVED_TAGS(context->received_tags,
                                          TAG_STRUCTURE_TYPE,
                                          TAG_STRUCTURE_VERSION,
                                          TAG_CONTACT_NAME,
                                          TAG_ADDRESS_NAME,
                                          TAG_ADDRESS_RAW,
                                          TAG_BLOCKCHAIN_FAMILY,
                                          TAG_DER_SIGNATURE);
    if (!result) {
        PRINTF("Missing mandatory fields in External Address descriptor!\n");
        return false;
    }
    if (context->external_address->blockchain_family == FAMILY_ETHEREUM) {
        result = TLV_CHECK_RECEIVED_TAGS(context->received_tags, TAG_CHAIN_ID);
        if (!result) {
            PRINTF("Missing mandatory fields in External Address descriptor!\n");
            return false;
        }
    }
    return true;
}

/**
 * @brief Print the received descriptor.
 *
 * @param[in] context the received payload
 * Only for debug purpose.
 */
static void print_payload(const s_ext_addr_ctx *context)
{
    UNUSED(context);
    PRINTF("****************************************************************************\n");
    PRINTF("[External Address] - Retrieved Descriptor:\n");
    PRINTF("[External Address] -    Contact name: %s\n", context->external_address->contact_name);
    PRINTF("[External Address] -    Address name: %s\n", context->external_address->address_name);
    PRINTF("[External Address] -    Address: %.*h\n",
           context->external_address->address_length,
           context->external_address->address_raw);
    PRINTF("[External Address] -    Blockchain family: %d (%s)\n",
           context->external_address->blockchain_family,
           FAMILY_STR(context->external_address->blockchain_family));
    if (context->external_address->blockchain_family == FAMILY_ETHEREUM) {
        PRINTF("[External Address] -    Chain ID: %llu\n", context->external_address->chain_id);
    }
}

/**
 * @brief Verify the struct
 *
 * Verify the SHA-256 hash of the payload against the public key
 *
 * @param[in] context the received payload
 * @return whether it was successful
 */
static bool verify_struct(const s_ext_addr_ctx *context)
{
    if (!verify_fields(context)) {
        PRINTF("Error: Missing mandatory fields in External Address descriptor!\n");
        return false;
    }

    if (!verify_signature(context)) {
        PRINTF("Error: Signature verification failed for External Address descriptor!\n");
        return false;
    }
    print_payload(context);
    return true;
}

/**
 * @brief Parse the TLV payload containing the External Address.
 *
 * @param[in] payload buffer_t with the complete TLV payload
 * @return whether the TLV payload was handled successfully or not
 */
bool handle_ext_addr_tlv_payload(const buffer_t *payload)
{
    bool           ret = false;
    s_ext_addr_ctx ctx = {0};

    // Init the structure
    ctx.external_address = &EXT_ADDR;
    // Initialize the hash context
    cx_sha256_init(&ctx.hash_ctx);

    // Parse using SDK TLV parser
    ret = ext_addr_tlv_parser(payload, &ctx, &ctx.received_tags);
    if (ret) {
        ret = verify_struct(&ctx);
    }
    return ret;
}

/* Exported functions --------------------------------------------------------*/
bolos_err_t add_external_address(uint8_t *buffer_in, size_t buffer_in_length)
{
    const buffer_t payload = {.ptr = buffer_in, .size = buffer_in_length};
    s_ext_addr_ctx ctx     = {0};

    // Init the structure
    ctx.external_address = &EXT_ADDR;
    // Initialize the hash context
    cx_sha256_init(&ctx.hash_ctx);

    // Parse using SDK TLV parser
    if (!ext_addr_tlv_parser(&payload, &ctx, &ctx.received_tags)) {
        return SWO_INCORRECT_DATA;
    }
    if (!verify_struct(&ctx)) {
        return SWO_SECURITY_CONDITION_NOT_SATISFIED;
    }

    // Check the address validity according to the Coin application logic
    if (!handle_check_external_address(ctx.external_address)) {
        PRINTF("Error: External Address is not valid according to the Coin application logic!\n");
        return SWO_WRONG_PARAMETER_VALUE;
    }

    return SWO_NO_RESPONSE;
}
#endif  // HAVE_ADDRESS_BOOK
