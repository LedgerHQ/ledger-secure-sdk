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

/**
 * @file identity_provide_contact.c
 * @brief Provide Contact flow (P1=0x20, struct type 0x33)
 *
 * Delivers a previously registered contact to the coin application so that
 * the application can substitute the human-readable name for a raw identifier
 * (e.g. an Ethereum address) during transaction review.
 *
 * Flow:
 *  1. Receive fully assembled payload (multi-chunk reassembly handled by
 *     address_book.c)
 *  2. Parse TLV payload (contact_name + scope + identifier +
 *     group_handle + derivation_path + blockchain_family [+ chain_id] +
 *     hmac_proof + hmac_rest)
 *  3. Verify group_handle → extract gid
 *  4. Verify HMAC_PROOF over (gid, contact_name)
 *  5. Verify HMAC_REST over (gid, scope, identifier, family[, chain_id])
 *  6. Call handle_provide_contact() so the coin app can store the contact
 *  7. Return SWO_SUCCESS (9000, no data)
 *
 * Active under HAVE_ADDRESS_BOOK.
 */

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "os_utils.h"
#include "status_words.h"
#include "tlv_library.h"
#include "buffer.h"
#include "bip32.h"
#include "address_book.h"
#include "address_book_entrypoints.h"
#include "address_book_crypto.h"
#include "address_book_common.h"
#include "identity.h"

#if defined(HAVE_ADDRESS_BOOK)

/* Private defines------------------------------------------------------------*/
#define STRUCT_VERSION 0x01

/* Private types, structures, unions -----------------------------------------*/
typedef struct {
    identity_t     *identity;
    TLV_reception_t received_tags;
    uint8_t         hmac_proof[CX_SHA256_SIZE];  ///< HMAC_PROOF — verifies contact name
    uint8_t         hmac_rest[CX_SHA256_SIZE];   ///< HMAC_REST  — verifies scope + identifier
    uint8_t         group_handle[GROUP_HANDLE_SIZE];  ///< Group handle from wallet (verified later)
} s_provide_contact_ctx;

/* Private macros-------------------------------------------------------------*/
#define PROVIDE_CONTACT_TAGS(X)                                                  \
    X(0x01, TAG_STRUCTURE_TYPE, handle_struct_type, ENFORCE_UNIQUE_TAG)          \
    X(0x02, TAG_STRUCTURE_VERSION, handle_struct_version, ENFORCE_UNIQUE_TAG)    \
    X(0xf0, TAG_CONTACT_NAME, handle_contact_name, ENFORCE_UNIQUE_TAG)           \
    X(0xf1, TAG_SCOPE, handle_scope, ENFORCE_UNIQUE_TAG)                         \
    X(0xf2, TAG_ACCOUNT_IDENTIFIER, handle_identifier, ENFORCE_UNIQUE_TAG)       \
    X(0xf6, TAG_GROUP_HANDLE, handle_group_handle, ENFORCE_UNIQUE_TAG)           \
    X(0x21, TAG_DERIVATION_PATH, handle_derivation_path, ENFORCE_UNIQUE_TAG)     \
    X(0x23, TAG_CHAIN_ID, handle_chain_id, ENFORCE_UNIQUE_TAG)                   \
    X(0x51, TAG_BLOCKCHAIN_FAMILY, handle_blockchain_family, ENFORCE_UNIQUE_TAG) \
    X(0x29, TAG_HMAC_NAME, handle_hmac_proof, ENFORCE_UNIQUE_TAG)                \
    X(0xf7, TAG_HMAC_REST, handle_hmac_rest, ENFORCE_UNIQUE_TAG)

/* Private variables ---------------------------------------------------------*/
static identity_t PROVIDE_CONTACT_IDENTITY = {0};

/* Private functions ---------------------------------------------------------*/

/**
 * @brief Handler for tag \ref STRUCTURE_TYPE
 *
 * @param[in] data    the tlv data
 * @param[in] context the received payload
 * @return whether the handling was successful
 */
static bool handle_struct_type(const tlv_data_t *data, s_provide_contact_ctx *context)
{
    UNUSED(context);
    if (!tlv_enforce_u8_value(data, TYPE_PROVIDE_CONTACT)) {
        PRINTF("[Provide Contact] Invalid STRUCTURE_TYPE value\n");
        return false;
    }
    return true;
}

/**
 * @brief Handler for tag \ref STRUCTURE_VERSION
 *
 * @param[in] data    the tlv data
 * @param[in] context the received payload
 * @return whether the handling was successful
 */
static bool handle_struct_version(const tlv_data_t *data, s_provide_contact_ctx *context)
{
    UNUSED(context);
    if (!tlv_enforce_u8_value(data, STRUCT_VERSION)) {
        PRINTF("[Provide Contact] Invalid STRUCTURE_VERSION value\n");
        return false;
    }
    return true;
}

/**
 * @brief Handler for tag \ref CONTACT_NAME
 *
 * @param[in]  data    the tlv data
 * @param[out] context the received payload
 * @return whether the handling was successful
 */
static bool handle_contact_name(const tlv_data_t *data, s_provide_contact_ctx *context)
{
    if (!address_book_handle_printable_string(
            data, context->identity->contact_name, sizeof(context->identity->contact_name))) {
        PRINTF("[Provide Contact] CONTACT_NAME: failed to parse\n");
        return false;
    }
    return true;
}

/**
 * @brief Handler for tag \ref SCOPE
 *
 * @param[in]  data    the tlv data
 * @param[out] context the received payload
 * @return whether the handling was successful
 */
static bool handle_scope(const tlv_data_t *data, s_provide_contact_ctx *context)
{
    if (!address_book_handle_printable_string(
            data, context->identity->scope, sizeof(context->identity->scope))) {
        PRINTF("[Provide Contact] SCOPE: failed to parse\n");
        return false;
    }
    return true;
}

/**
 * @brief Handler for tag \ref IDENTIFIER
 *
 * @param[in]  data    the tlv data
 * @param[out] context the received payload
 * @return whether the handling was successful
 */
static bool handle_identifier(const tlv_data_t *data, s_provide_contact_ctx *context)
{
    buffer_t buf = {0};
    if (!get_buffer_from_tlv_data(data, &buf, 1, IDENTIFIER_MAX_LENGTH)) {
        PRINTF("[Provide Contact] IDENTIFIER: failed to extract\n");
        return false;
    }
    memmove(context->identity->identifier, buf.ptr, buf.size);
    context->identity->identifier_len = (uint8_t) buf.size;
    return true;
}

/**
 * @brief Handler for tag \ref GROUP_HANDLE
 *
 * @param[in]  data    the tlv data
 * @param[out] context the received payload
 * @return whether the handling was successful
 */
static bool handle_group_handle(const tlv_data_t *data, s_provide_contact_ctx *context)
{
    buffer_t buf = {0};
    if (!get_buffer_from_tlv_data(data, &buf, GROUP_HANDLE_SIZE, GROUP_HANDLE_SIZE)) {
        PRINTF("[Provide Contact] GROUP_HANDLE: invalid length (expected %d bytes)\n",
               GROUP_HANDLE_SIZE);
        return false;
    }
    memmove(context->group_handle, buf.ptr, GROUP_HANDLE_SIZE);
    return true;
}

/**
 * @brief Handler for tag \ref DERIVATION_PATH
 *
 * @param[in]  data    the tlv data
 * @param[out] context the received payload
 * @return whether the handling was successful
 */
static bool handle_derivation_path(const tlv_data_t *data, s_provide_contact_ctx *context)
{
    return address_book_handle_derivation_path(data, &context->identity->bip32_path);
}

/**
 * @brief Handler for tag \ref CHAIN_ID
 *
 * @param[in]  data    the tlv data
 * @param[out] context the received payload
 * @return whether the handling was successful
 */
static bool handle_chain_id(const tlv_data_t *data, s_provide_contact_ctx *context)
{
    return address_book_handle_chain_id(data, &context->identity->chain_id);
}

/**
 * @brief Handler for tag \ref BLOCKCHAIN_FAMILY
 *
 * @param[in]  data    the tlv data
 * @param[out] context the received payload
 * @return whether the handling was successful
 */
static bool handle_blockchain_family(const tlv_data_t *data, s_provide_contact_ctx *context)
{
    return address_book_handle_blockchain_family(data, &context->identity->blockchain_family);
}

/**
 * @brief Handler for tag \ref HMAC_PROOF (TAG 0x54)
 *
 * @param[in]  data    the tlv data
 * @param[out] context the received payload
 * @return whether the handling was successful
 */
static bool handle_hmac_proof(const tlv_data_t *data, s_provide_contact_ctx *context)
{
    buffer_t buf = {0};
    if (!get_buffer_from_tlv_data(data, &buf, CX_SHA256_SIZE, CX_SHA256_SIZE)) {
        PRINTF("[Provide Contact] HMAC_PROOF: invalid length (expected %d bytes)\n",
               CX_SHA256_SIZE);
        return false;
    }
    memmove(context->hmac_proof, buf.ptr, CX_SHA256_SIZE);
    return true;
}

/**
 * @brief Handler for tag \ref HMAC_REST (TAG 0x55)
 *
 * @param[in]  data    the tlv data
 * @param[out] context the received payload
 * @return whether the handling was successful
 */
static bool handle_hmac_rest(const tlv_data_t *data, s_provide_contact_ctx *context)
{
    buffer_t buf = {0};
    if (!get_buffer_from_tlv_data(data, &buf, CX_SHA256_SIZE, CX_SHA256_SIZE)) {
        PRINTF("[Provide Contact] HMAC_REST: invalid length (expected %d bytes)\n", CX_SHA256_SIZE);
        return false;
    }
    memmove(context->hmac_rest, buf.ptr, CX_SHA256_SIZE);
    return true;
}

DEFINE_TLV_PARSER(PROVIDE_CONTACT_TAGS, NULL, provide_contact_tlv_parser)

/**
 * @brief Check that all mandatory TLV tags were received
 *
 * @param[in] context the received payload
 * @return whether all mandatory fields are present
 */
static bool verify_fields(const s_provide_contact_ctx *context)
{
    bool result = TLV_CHECK_RECEIVED_TAGS(context->received_tags,
                                          TAG_STRUCTURE_TYPE,
                                          TAG_STRUCTURE_VERSION,
                                          TAG_CONTACT_NAME,
                                          TAG_SCOPE,
                                          TAG_ACCOUNT_IDENTIFIER,
                                          TAG_GROUP_HANDLE,
                                          TAG_DERIVATION_PATH,
                                          TAG_BLOCKCHAIN_FAMILY,
                                          TAG_HMAC_NAME,
                                          TAG_HMAC_REST);
    if (!result) {
        PRINTF("[Provide Contact] Missing mandatory fields!\n");
        return false;
    }
    if (context->identity->blockchain_family == FAMILY_ETHEREUM) {
        result = TLV_CHECK_RECEIVED_TAGS(context->received_tags, TAG_CHAIN_ID);
        if (!result) {
            PRINTF("[Provide Contact] Missing CHAIN_ID for Ethereum family!\n");
            return false;
        }
    }
    return true;
}

/**
 * @brief Print the received descriptor
 *
 * @param[in] context the received payload
 * Only for debug purpose.
 */
static void print_payload(const s_provide_contact_ctx *context)
{
    UNUSED(context);
    PRINTF("****************************************************************************\n");
    PRINTF("[Provide Contact] - Retrieved Descriptor:\n");
    PRINTF("[Provide Contact] -    Contact name:   %s\n", context->identity->contact_name);
    if (context->identity->scope[0] != '\0') {
        PRINTF("[Provide Contact] -    Scope:          %s\n", context->identity->scope);
    }
    PRINTF("[Provide Contact] -    Identifier len: %d\n", context->identity->identifier_len);
    PRINTF("[Provide Contact] -    Blockchain:     %s\n",
           FAMILY_AS_STR(context->identity->blockchain_family));
}

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Deliver a verified contact to the coin application.
 *
 * Parses, verifies, and passes the contact to the coin app callback.
 * Returns SWO_SUCCESS (9000) with no response data on success.
 *
 * @param[in] buffer_in        the fully assembled TLV payload
 * @param[in] buffer_in_length the length of the payload
 * @return bolos_err_t         the result of the command
 */
bolos_err_t provide_contact(uint8_t *buffer_in, size_t buffer_in_length)
{
    const buffer_t        payload = {.ptr = buffer_in, .size = buffer_in_length};
    s_provide_contact_ctx ctx     = {0};

    ctx.identity = &PROVIDE_CONTACT_IDENTITY;

    // Parse using SDK TLV parser
    if (!provide_contact_tlv_parser(&payload, &ctx, &ctx.received_tags)) {
        PRINTF("[Provide Contact] TLV parsing failed\n");
        return SWO_INCORRECT_DATA;
    }
    if (!verify_fields(&ctx)) {
        return SWO_INCORRECT_DATA;
    }
    print_payload(&ctx);

    // Verify the group handle and extract the gid
    if (!address_book_verify_group_handle(
            &PROVIDE_CONTACT_IDENTITY.bip32_path, ctx.group_handle, PROVIDE_CONTACT_IDENTITY.gid)) {
        PRINTF("[Provide Contact] Group handle verification failed\n");
        return SWO_CONDITIONS_NOT_SATISFIED;
    }

    // Verify HMAC_PROOF over (gid, contact_name)
    if (!address_book_verify_hmac_name(&PROVIDE_CONTACT_IDENTITY.bip32_path,
                                       PROVIDE_CONTACT_IDENTITY.gid,
                                       PROVIDE_CONTACT_IDENTITY.contact_name,
                                       ctx.hmac_proof)) {
        PRINTF("[Provide Contact] HMAC_PROOF verification failed\n");
        return SWO_CONDITIONS_NOT_SATISFIED;
    }

    // Verify HMAC_REST over (gid, scope, identifier, family[, chain_id])
    if (!address_book_verify_hmac_rest(&PROVIDE_CONTACT_IDENTITY.bip32_path,
                                       PROVIDE_CONTACT_IDENTITY.gid,
                                       PROVIDE_CONTACT_IDENTITY.scope,
                                       PROVIDE_CONTACT_IDENTITY.identifier,
                                       PROVIDE_CONTACT_IDENTITY.identifier_len,
                                       PROVIDE_CONTACT_IDENTITY.blockchain_family,
                                       PROVIDE_CONTACT_IDENTITY.chain_id,
                                       ctx.hmac_rest)) {
        PRINTF("[Provide Contact] HMAC_REST verification failed\n");
        return SWO_CONDITIONS_NOT_SATISFIED;
    }

    // Pass verified contact to the coin app for storage
    if (!handle_provide_contact(&PROVIDE_CONTACT_IDENTITY)) {
        PRINTF("[Provide Contact] Rejected by coin application\n");
        return SWO_WRONG_PARAMETER_VALUE;
    }

    return SWO_SUCCESS;
}

#endif  // HAVE_ADDRESS_BOOK
