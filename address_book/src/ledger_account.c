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
#include "ledger_account.h"

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
    X(0x15, TAG_DER_SIGNATURE, handle_signature, ENFORCE_UNIQUE_TAG)

/* Private functions prototypes ----------------------------------------------*/
static bool common_handler(const tlv_data_t *data, s_ledger_account_ctx *context);

/* Exported variables --------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
// Global structure to store the Ledger Account
static ledger_account_t LEDGER_ACCOUNT = {0};

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
    // Extract the string (with null terminator added by get_string_from_tlv_data)
    if (!get_string_from_tlv_data(data,
                                  (char *) context->ledger_account->account_name,
                                  1,
                                  sizeof(context->ledger_account->account_name) - 1)) {
        PRINTF("ACCOUNT_NAME: failed to extract\n");
        return false;
    }
    if (!is_printable_string((const char *) context->ledger_account->account_name,
                             strlen((const char *) context->ledger_account->account_name))) {
        PRINTF("ACCOUNT_NAME: error\n");
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
    buffer_t path_buffer = {0};
    size_t   nb_elements = 0;
    // Get the raw buffer
    if (!get_buffer_from_tlv_data(data, &path_buffer, 4, MAX_BIP32_PATH * 4)) {
        PRINTF("DERIVATION_PATH: failed to extract\n");
        return false;
    }
    nb_elements = path_buffer.ptr[0];  // First byte is the length in bytes
    path_buffer.ptr++;                 // Skip the first byte which is the length in bytes
    path_buffer.size--;                // Adjust the size accordingly

    // Check BIP32 length
    if (path_buffer.size < sizeof(uint32_t) * nb_elements) {
        PRINTF("DERIVATION_PATH: Invalid data\n");
        return false;
    }

    // Parse BIP32 path
    if (!bip32_path_read(path_buffer.ptr,
                         path_buffer.size,
                         (uint32_t *) context->ledger_account->derivation_path,
                         nb_elements)) {
        PRINTF("DERIVATION_PATH: failed to parse BIP32 path\n");
        return false;
    }
    // Store the path length (number of elements)
    context->ledger_account->derivation_path_length = path_buffer.size / 4;
    return true;
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
    if (!get_uint64_t_from_tlv_data(data, &context->ledger_account->chain_id)) {
        PRINTF("CHAIN_ID: failed to extract\n");
        return false;
    }
    return true;
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
 * @brief Define the TLV parser for Ledger Account.
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
static bool verify_signature(const s_ledger_account_ctx *context)
{
    uint8_t          hash[CX_SHA256_SIZE] = {0};
    const buffer_t   buffer               = {.ptr = hash, .size = sizeof(hash)};
    const buffer_t   signature = {.ptr = (uint8_t *) context->sig, .size = context->sig_size};
    const cx_curve_t curve     = CX_CURVE_SECP256R1;
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
static bool verify_fields(const s_ledger_account_ctx *context)
{
    bool result = TLV_CHECK_RECEIVED_TAGS(context->received_tags,
                                          TAG_STRUCTURE_TYPE,
                                          TAG_STRUCTURE_VERSION,
                                          TAG_ACCOUNT_NAME,
                                          TAG_DERIVATION_PATH,
                                          TAG_DERIVATION_SCHEME,
                                          TAG_DER_SIGNATURE);
    if (!result) {
        PRINTF("Missing mandatory fields in Ledger Account descriptor!\n");
        return false;
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
    if (bip32_path_format(context->ledger_account->derivation_path,
                          context->ledger_account->derivation_path_length,
                          out,
                          sizeof(out))) {
        PRINTF("[Ledger Account] -    Derivation path[%d]: %s\n",
               context->ledger_account->derivation_path_length,
               out);
    }
    else {
        PRINTF("[Ledger Account] -    Derivation path length[%d] (failed to format)\n",
               context->ledger_account->derivation_path_length);
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
        PRINTF("Failed to verify Ledger Account structure!\n");
        return SWO_SECURITY_CONDITION_NOT_SATISFIED;
    }

    // Check the address validity according to the Coin application logic
    if (!handle_check_ledger_account(ctx.ledger_account)) {
        PRINTF("Error: Ledger Account is not valid according to the Coin application logic!\n");
        return SWO_WRONG_PARAMETER_VALUE;
    }

    return SWO_NO_RESPONSE;
}

#endif  // HAVE_ADDRESS_BOOK
