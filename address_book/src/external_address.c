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
#include "bip32.h"
#include "address_book.h"
#include "address_book_entrypoints.h"
#include "address_book_crypto.h"
#include "io.h"
#include "external_address.h"
#include "address_book_common.h"
#include "nbgl_use_case.h"

#if defined(HAVE_ADDRESS_BOOK)

/* Private defines------------------------------------------------------------*/
#define STRUCT_VERSION 0x01

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
// Define TLV tags for External Address
#define EXTERNAL_ADDRESS_TAGS(X)                                                 \
    X(0x01, TAG_STRUCTURE_TYPE, handle_struct_type, ENFORCE_UNIQUE_TAG)          \
    X(0x02, TAG_STRUCTURE_VERSION, handle_struct_version, ENFORCE_UNIQUE_TAG)    \
    X(0x0A, TAG_CONTACT_NAME, handle_contact_name, ENFORCE_UNIQUE_TAG)           \
    X(0x0B, TAG_ADDRESS_NAME, handle_address_name, ENFORCE_UNIQUE_TAG)           \
    X(0x21, TAG_DERIVATION_PATH, handle_derivation_path, ENFORCE_UNIQUE_TAG)     \
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
// UI variables
static nbgl_contentTagValueList_t ui_pairsList = {0};

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
    if (!address_book_handle_printable_string(data,
                                              (char *) context->external_address->contact_name,
                                              sizeof(context->external_address->contact_name))) {
        PRINTF("CONTACT_NAME: failed to parse\n");
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
    if (!address_book_handle_printable_string(data,
                                              (char *) context->external_address->address_name,
                                              sizeof(context->external_address->address_name))) {
        PRINTF("ADDRESS_NAME: failed to parse\n");
        return false;
    }
    return true;
}

/**
 * @brief Handler for tag \ref DERIVATION_PATH.
 *
 * @param[in] data the tlv data
 * @param[in] context the received payload
 * @return whether the handling was successful
 */
static bool handle_derivation_path(const tlv_data_t *data, s_ext_addr_ctx *context)
{
    return address_book_handle_derivation_path(data, &context->external_address->bip32_path);
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
    return address_book_handle_chain_id(data, &context->external_address->chain_id);
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
    return address_book_handle_blockchain_family(data,
                                                 &context->external_address->blockchain_family);
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
    return address_book_handle_signature(data, &context->sig, &context->sig_size);
}

/**
 * @brief Define the TLV parser
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
    return address_book_hash_handler(data, &context->hash_ctx);
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
    return address_book_verify_signature(
        (cx_sha256_t *) &context->hash_ctx, context->sig, context->sig_size);
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
                                          TAG_DERIVATION_PATH,
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
    char out[50] = {0};

    PRINTF("****************************************************************************\n");
    PRINTF("[External Address] - Retrieved Descriptor:\n");
    PRINTF("[External Address] -    Contact name: %s\n", context->external_address->contact_name);
    PRINTF("[External Address] -    Address name: %s\n", context->external_address->address_name);
    if (bip32_path_format_simple(&context->external_address->bip32_path, out, sizeof(out))) {
        PRINTF("[External Address] -    Derivation path[%d]: %s\n",
               context->external_address->bip32_path.length,
               out);
    }
    else {
        PRINTF("[External Address] -    Derivation path length[%d] (failed to format)\n",
               context->external_address->bip32_path.length);
    }
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
 * @brief Build the remaining payload (raw data with explicit lengths)
 *
 * Format: len(1) | address_name | len(1) | address_raw | chain_id(8) | family(1)
 *
 * @param[out] buffer Output buffer
 * @param[out] length Length of the payload
 * @return true if successful
 */
static bool build_remaining_payload(uint8_t *buffer, size_t *length)
{
    size_t  offset           = 0;
    uint8_t address_name_len = strlen(EXT_ADDR.address_name);

    // Address name length + data
    buffer[offset++] = address_name_len;
    memmove(&buffer[offset], EXT_ADDR.address_name, address_name_len);
    offset += address_name_len;

    // Address length + raw data
    buffer[offset++] = EXT_ADDR.address_length;
    memmove(&buffer[offset], EXT_ADDR.address_raw, EXT_ADDR.address_length);
    offset += EXT_ADDR.address_length;

    // Chain ID (8 bytes, big endian)
    U8BE_ENCODE(buffer, offset, EXT_ADDR.chain_id);
    offset += 8;

    // Blockchain family (1 byte)
    buffer[offset++] = (uint8_t) EXT_ADDR.blockchain_family;

    *length = offset;
    return true;
}

/**
 * @brief Build and send encrypted response
 *
 * This function constructs the External Address entry payload and delegates
 * to the generic crypto function for signing, encryption and sending.
 *
 * @return true if successful, false otherwise
 */
static bool build_and_send_response(void)
{
    uint8_t name_payload[EXTERNAL_NAME_LENGTH] = {0};
    size_t  name_payload_len;

    uint8_t remaining_payload[MAX_EXTERNAL_REMAINING] = {0};
    size_t  remaining_payload_len                     = 0;

    // Build name payload
    if (!build_name_payload(
            EXT_ADDR.contact_name, name_payload, sizeof(name_payload), &name_payload_len)) {
        PRINTF("Error: Failed to build name payload\n");
        return false;
    }

    // Build remaining payload
    if (!build_remaining_payload(remaining_payload, &remaining_payload_len)) {
        PRINTF("Error: Failed to build remaining payload\n");
        return false;
    }

    // Use derivation path from TLV for encryption
    return address_book_encrypt_and_send(TYPE_EXTERNAL_ADDRESS,
                                         &EXT_ADDR.bip32_path,
                                         name_payload,
                                         name_payload_len,
                                         remaining_payload,
                                         remaining_payload_len);
}

/**
 * @brief Callback when user confirms or rejects the External Address
 *
 * @param[in] confirm true if user confirmed, false if rejected
 */
static void review_external_address_choice(bool confirm)
{
    if (confirm) {
        // Build and send encrypted response to host
        if (build_and_send_response()) {
            nbgl_useCaseStatus("Saved to your contacts", true, finalize_ui_external_address);
        }
        else {
            PRINTF("Error: Failed to build and send response\n");
            io_send_sw(SWO_INCORRECT_DATA);
            nbgl_useCaseStatus("Error saving contact", false, finalize_ui_external_address);
        }
    }
    else {
        io_send_sw(SWO_INCORRECT_DATA);
        nbgl_useCaseReviewStatus(STATUS_TYPE_OPERATION_REJECTED, finalize_ui_external_address);
    }
}

/**
 * @brief Display External Address confirmation flow
 */
static void ui_display_external_address(void)
{
    memset(&ui_pairsList, 0, sizeof(ui_pairsList));
    ui_pairsList.nbPairs  = 4;
    ui_pairsList.callback = get_external_address_tagValue;
    ui_pairsList.wrapping = true;

    // Display the review screen
    nbgl_useCaseReviewLight(TYPE_OPERATION,
                            &ui_pairsList,
                            &LARGE_ADDRESS_BOOK_ICON,
                            "Review contact details",
                            NULL,
                            "Confirm contact details?",
                            review_external_address_choice);
}

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Handle add External Address operation
 *
 * Parses the TLV payload, verifies the signature, checks the address validity and stores it.
 *
 * @param[in] buffer_in Complete TLV payload
 * @param[in] buffer_in_length Payload length
 * @return Status word
 */
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

    // Display confirmation UI
    ui_display_external_address();

    return SWO_NO_RESPONSE;
}
#endif  // HAVE_ADDRESS_BOOK
