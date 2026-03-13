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
#include "ledger_account.h"
#include "address_book_common.h"
#include "nbgl_use_case.h"

#if defined(HAVE_ADDRESS_BOOK)

/* Private defines------------------------------------------------------------*/
#define STRUCT_VERSION 0x01

/* Private enumerations ------------------------------------------------------*/

/* Private types, structures, unions -----------------------------------------*/
typedef struct {
    ledger_account_t *ledger_account;
    uint8_t           sig_size;
    const uint8_t    *sig;
    cx_sha256_t       hash_ctx;
    TLV_reception_t   received_tags;
} s_ledger_account_ctx;

/* Private macros-------------------------------------------------------------*/
// Define TLV tags for Ledger Account
#define LEDGER_ACCOUNT_TAGS(X)                                                   \
    X(0x01, TAG_STRUCTURE_TYPE, handle_struct_type, ENFORCE_UNIQUE_TAG)          \
    X(0x02, TAG_STRUCTURE_VERSION, handle_struct_version, ENFORCE_UNIQUE_TAG)    \
    X(0x0C, TAG_ACCOUNT_NAME, handle_account_name, ENFORCE_UNIQUE_TAG)           \
    X(0x21, TAG_DERIVATION_PATH, handle_derivation_path, ENFORCE_UNIQUE_TAG)     \
    X(0x50, TAG_DERIVATION_SCHEME, handle_derivation_scheme, ENFORCE_UNIQUE_TAG) \
    X(0x23, TAG_CHAIN_ID, handle_chain_id, ENFORCE_UNIQUE_TAG)                   \
    X(0x51, TAG_BLOCKCHAIN_FAMILY, handle_blockchain_family, ENFORCE_UNIQUE_TAG) \
    X(0x15, TAG_DER_SIGNATURE, handle_signature, ENFORCE_UNIQUE_TAG)

/* Private functions prototypes ----------------------------------------------*/
static bool common_handler(const tlv_data_t *data, s_ledger_account_ctx *context);

/* Exported variables --------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
// Global structure to store the Ledger Account
static ledger_account_t LEDGER_ACCOUNT = {0};
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
static bool handle_struct_type(const tlv_data_t *data, s_ledger_account_ctx *context)
{
    UNUSED(context);
    return tlv_check_structure_type(data, TYPE_LEDGER_ACCOUNT);
}

/**
 * @brief Handler for tag \ref STRUCTURE_VERSION.
 *
 * @param[in] data the tlv data
 * @param[in] context the received payload
 * @return whether the handling was successful
 */
static bool handle_struct_version(const tlv_data_t *data, s_ledger_account_ctx *context)
{
    UNUSED(context);
    return tlv_check_structure_version(data, STRUCT_VERSION);
}

/**
 * @brief Handler for tag \ref ACCOUNT_NAME.
 *
 * @param[in] data the tlv data
 * @param[in] context the received payload
 * @return whether the handling was successful
 */
static bool handle_account_name(const tlv_data_t *data, s_ledger_account_ctx *context)
{
    if (!address_book_handle_printable_string(data,
                                              (char *) context->ledger_account->account_name,
                                              sizeof(context->ledger_account->account_name))) {
        PRINTF("ACCOUNT_NAME: failed to parse\n");
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
static bool handle_derivation_path(const tlv_data_t *data, s_ledger_account_ctx *context)
{
    return address_book_handle_derivation_path(data, &context->ledger_account->bip32_path);
}

/**
 * @brief Handler for tag \ref DERIVATION_SCHEME.
 *
 * @param[in] data the tlv data
 * @param[in] context the received payload
 * @return whether the handling was successful
 */
static bool handle_derivation_scheme(const tlv_data_t *data, s_ledger_account_ctx *context)
{
    uint8_t value = 0;
    if (!get_uint8_t_from_tlv_data(data, &value)) {
        PRINTF("DERIVATION_SCHEME: failed to extract\n");
        return false;
    }

    // Validate derivation scheme value
    if (value < DERIVATION_SCHEME_BIP44 || value > DERIVATION_SCHEME_GENERIC) {
        PRINTF("DERIVATION_SCHEME: Value out of range (%d)!\n", value);
        return false;
    }

    context->ledger_account->derivation_scheme = (derivation_scheme_e) value;
    return true;
}

/**
 * @brief Parse the CHAIN_ID value.
 *
 * @param[in] data data to handle
 * @param[out] context the received payload
 * @return whether the handling was successful
 */
static bool handle_chain_id(const tlv_data_t *data, s_ledger_account_ctx *context)
{
    return address_book_handle_chain_id(data, &context->ledger_account->chain_id);
}

/**
 * @brief Parse the BLOCKCHAIN_FAMILY value.
 *
 * @param[in] data data to handle
 * @param[out] context the received payload
 * @return whether the handling was successful
 */
static bool handle_blockchain_family(const tlv_data_t *data, s_ledger_account_ctx *context)
{
    return address_book_handle_blockchain_family(data, &context->ledger_account->blockchain_family);
}

/**
 * Handler for tag \ref DER_SIGNATURE
 *
 * @param[in] data the tlv data
 * @param[in] context the received payload
 * @return whether the handling was successful
 */
static bool handle_signature(const tlv_data_t *data, s_ledger_account_ctx *context)
{
    return address_book_handle_signature(data, &context->sig, &context->sig_size);
}

/**
 * @brief Define the TLV parser
 *
 * This macro generates a TLV parser function named ledger_account_tlv_parser that processes
 * Ledger Account payloads according to the LEDGER_ACCOUNT_TAGS specification.
 * The parser iterates through TLV-encoded data, invokes specific tag handlers, and
 * calls the common_handler for additional processing (such as hashing).
 */
DEFINE_TLV_PARSER(LEDGER_ACCOUNT_TAGS, &common_handler, ledger_account_tlv_parser)

/**
 * @brief Common handler that hashes all tags except signature
 *
 * @param[in] data the tlv data
 * @param[in] context the received payload
 * @return true on success
 */
static bool common_handler(const tlv_data_t *data, s_ledger_account_ctx *context)
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
static bool verify_signature(const s_ledger_account_ctx *context)
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
static bool verify_fields(const s_ledger_account_ctx *context)
{
    bool result = TLV_CHECK_RECEIVED_TAGS(context->received_tags,
                                          TAG_STRUCTURE_TYPE,
                                          TAG_STRUCTURE_VERSION,
                                          TAG_ACCOUNT_NAME,
                                          TAG_DERIVATION_PATH,
                                          TAG_DERIVATION_SCHEME,
                                          TAG_BLOCKCHAIN_FAMILY,
                                          TAG_DER_SIGNATURE);
    if (!result) {
        PRINTF("Missing mandatory fields in Ledger Account descriptor!\n");
        return false;
    }
    if (context->ledger_account->blockchain_family == FAMILY_ETHEREUM) {
        result = TLV_CHECK_RECEIVED_TAGS(context->received_tags, TAG_CHAIN_ID);
        if (!result) {
            PRINTF("Missing mandatory fields in Ledger Account descriptor!\n");
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
static void print_payload(const s_ledger_account_ctx *context)
{
    UNUSED(context);
    char out[50] = {0};

    PRINTF("****************************************************************************\n");
    PRINTF("[Ledger Account] - Retrieved Descriptor:\n");
    PRINTF("[Ledger Account] -    Account name: %s\n", context->ledger_account->account_name);
    if (bip32_path_format_simple(&context->ledger_account->bip32_path, out, sizeof(out))) {
        PRINTF("[Ledger Account] -    Derivation path[%d]: %s\n",
               context->ledger_account->bip32_path.length,
               out);
    }
    else {
        PRINTF("[Ledger Account] -    Derivation path length[%d] (failed to format)\n",
               context->ledger_account->bip32_path.length);
    }
    PRINTF("[Ledger Account] -    Derivation scheme: %s\n",
           DERIVATION_SCHEME_STR(context->ledger_account->derivation_scheme));
    PRINTF("[Ledger Account] -    Chain ID: %llu\n", context->ledger_account->chain_id);
}

/**
 * @brief Verify the struct
 *
 * Verify the SHA-256 hash of the payload against the public key
 *
 * @param[in] context the received payload
 * @return whether it was successful
 */
static bool verify_struct(const s_ledger_account_ctx *context)
{
    if (!verify_fields(context)) {
        PRINTF("Error: Missing mandatory fields in Ledger Account descriptor!\n");
        return false;
    }

    if (!verify_signature(context)) {
        PRINTF("Error: Signature verification failed for Ledger Account descriptor!\n");
        return false;
    }
    print_payload(context);
    return true;
}

/**
 * @brief Build the remaining payload (raw data with explicit lengths)
 *
 * Format: len(1) | derivation_path[](4*N) | derivation_scheme(1) | chain_id(8) |
 * blockchain_family(1)
 *
 * @param[out] buffer Output buffer
 * @param[out] length Length of the payload
 * @return true if successful
 */
static bool build_remaining_payload(uint8_t *buffer, size_t *length)
{
    size_t offset = 0;

    // Derivation path length + data
    offset += bip32_path_encode(&LEDGER_ACCOUNT.bip32_path, buffer + offset);

    // Derivation scheme (1 byte)
    buffer[offset++] = (uint8_t) LEDGER_ACCOUNT.derivation_scheme;

    // Chain ID (8 bytes, big endian)
    U8BE_ENCODE(buffer, offset, LEDGER_ACCOUNT.chain_id);
    offset += 8;

    // Blockchain family (1 byte)
    buffer[offset++] = (uint8_t) LEDGER_ACCOUNT.blockchain_family;

    *length = offset;
    return true;
}

/**
 * @brief Build and send encrypted response
 *
 * This function constructs the Ledger Account entry payload and delegates
 * to the generic crypto function for signing, encryption and sending.
 *
 * @return true if successful, false otherwise
 */
static bool build_and_send_response(void)
{
    uint8_t name_payload[ACCOUNT_NAME_LENGTH] = {0};
    size_t  name_payload_len;

    uint8_t remaining_payload[MAX_LEDGER_REMAINING] = {0};
    size_t  remaining_payload_len                   = 0;

    // Build name payload
    if (!build_name_payload(
            LEDGER_ACCOUNT.account_name, name_payload, sizeof(name_payload), &name_payload_len)) {
        PRINTF("Error: Failed to build name payload\n");
        return false;
    }

    // Build remaining payload
    if (!build_remaining_payload(remaining_payload, &remaining_payload_len)) {
        PRINTF("Error: Failed to build remaining payload\n");
        return false;
    }

    // Use derivation path from TLV for encryption
    return address_book_encrypt_and_send(TYPE_LEDGER_ACCOUNT,
                                         &LEDGER_ACCOUNT.bip32_path,
                                         name_payload,
                                         name_payload_len,
                                         remaining_payload,
                                         remaining_payload_len);
}

/**
 * @brief Callback when user confirms or rejects the Ledger Account
 *
 * @param[in] confirm true if user confirmed, false if rejected
 */
static void review_ledger_account_choice(bool confirm)
{
    if (confirm) {
        // Build and send encrypted response to host
        if (build_and_send_response()) {
            nbgl_useCaseStatus("Saved to your accounts", true, finalize_ui_ledger_account);
        }
        else {
            PRINTF("Error: Failed to build and send response\n");
            io_send_sw(SWO_INCORRECT_DATA);
            nbgl_useCaseStatus("Error saving account", false, finalize_ui_ledger_account);
        }
    }
    else {
        io_send_sw(SWO_INCORRECT_DATA);
        nbgl_useCaseReviewStatus(STATUS_TYPE_OPERATION_REJECTED, finalize_ui_ledger_account);
    }
}

/**
 * @brief Display Ledger Account confirmation flow
 */
static void ui_display_ledger_account(void)
{
    const nbgl_icon_details_t *icon = NULL;
    memset(&ui_pairsList, 0, sizeof(ui_pairsList));
    ui_pairsList.nbPairs  = 2;
    ui_pairsList.callback = get_ledger_account_tagValue;
    ui_pairsList.wrapping = true;

    // Get the icon for the UI
    icon = get_ledger_account_icon();
    // Display the review screen
    nbgl_useCaseReviewLight(TYPE_OPERATION,
                            &ui_pairsList,
                            icon,
                            "Review account details",
                            NULL,
                            "Confirm account details?",
                            review_ledger_account_choice);
}

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Handle add Ledger Account operation
 *
 * Parses the TLV payload, verifies the signature, checks the account validity and stores it.
 *
 * @param[in] buffer_in Complete TLV payload
 * @param[in] buffer_in_length Payload length
 * @return Status word
 */
bolos_err_t add_ledger_account(uint8_t *buffer_in, size_t buffer_in_length)
{
    const buffer_t       payload = {.ptr = buffer_in, .size = buffer_in_length};
    s_ledger_account_ctx ctx     = {0};

    // Init the structure
    ctx.ledger_account = &LEDGER_ACCOUNT;
    // Initialize the hash context
    cx_sha256_init(&ctx.hash_ctx);

    // Parse using SDK TLV parser
    if (!ledger_account_tlv_parser(&payload, &ctx, &ctx.received_tags)) {
        PRINTF("Failed to parse Ledger Account TLV payload!\n");
        return SWO_INCORRECT_DATA;
    }
    if (!verify_struct(&ctx)) {
        return SWO_SECURITY_CONDITION_NOT_SATISFIED;
    }

    // Check the address validity according to the Coin application logic
    if (!handle_check_ledger_account(ctx.ledger_account)) {
        PRINTF("Error: Ledger Account is not valid according to the Coin application logic!\n");
        return SWO_WRONG_PARAMETER_VALUE;
    }

    // Display confirmation UI
    ui_display_ledger_account();

    return SWO_NO_RESPONSE;
}
#endif  // HAVE_ADDRESS_BOOK
