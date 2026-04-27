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
 * @file ledger_account_provide_contact.c
 * @brief Provide Ledger Account Contact flow (P1=0x21, struct type 0x34)
 *
 * Delivers a previously registered Ledger Account to the coin application so
 * that the application can substitute the human-readable account name for the
 * raw derived address during transaction review.
 *
 * Unlike the Identity Provide Contact flow (P1=0x20), a Ledger Account has no
 * external address, no group_handle, no scope, and no hmac_rest.  The only
 * proof required is the HMAC Proof of Registration returned by
 * register_ledger_account (P1=0x11).
 *
 * Flow:
 *  1. Receive fully assembled payload (multi-chunk reassembly handled by
 *     address_book.c)
 *  2. Parse TLVpayload (contact_name + derivation_path + chain_id + blockchain_family
 *     + hmac_proof)
 *  3. Verify HMAC Proof of Registration
 *  4. Call handle_provide_ledger_account_contact() so the coin app can store the contact
 *  5. Return SWO_SUCCESS (9000, no data)
 *
 * Active under HAVE_ADDRESS_BOOK && HAVE_ADDRESS_BOOK_LEDGER_ACCOUNT.
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
#include "ledger_account.h"

#if defined(HAVE_ADDRESS_BOOK) && defined(HAVE_ADDRESS_BOOK_LEDGER_ACCOUNT)

/* Private defines------------------------------------------------------------*/
#define STRUCT_VERSION 0x01

/* Private types, structures, unions -----------------------------------------*/
typedef struct {
    ledger_account_t *ledger_account;
    TLV_reception_t   received_tags;
    uint8_t           hmac_proof[CX_SHA256_SIZE];
} s_provide_ledger_account_ctx;

/* Private macros-------------------------------------------------------------*/
#define PROVIDE_LEDGER_ACCOUNT_TAGS(X)                                           \
    X(0x01, TAG_STRUCTURE_TYPE, handle_struct_type, ENFORCE_UNIQUE_TAG)          \
    X(0x02, TAG_STRUCTURE_VERSION, handle_struct_version, ENFORCE_UNIQUE_TAG)    \
    X(0xf0, TAG_CONTACT_NAME, handle_contact_name, ENFORCE_UNIQUE_TAG)           \
    X(0x21, TAG_DERIVATION_PATH, handle_derivation_path, ENFORCE_UNIQUE_TAG)     \
    X(0x23, TAG_CHAIN_ID, handle_chain_id, ENFORCE_UNIQUE_TAG)                   \
    X(0x51, TAG_BLOCKCHAIN_FAMILY, handle_blockchain_family, ENFORCE_UNIQUE_TAG) \
    X(0x29, HMAC_PROOF, handle_hmac_proof, ENFORCE_UNIQUE_TAG)

/* Private variables ---------------------------------------------------------*/
static ledger_account_t PROVIDE_LEDGER_ACCOUNT = {0};

/* Private functions ---------------------------------------------------------*/

/**
 * @brief Handler for tag \ref STRUCTURE_TYPE
 *
 * @param[in] data    the tlv data
 * @param[in] context the received payload
 * @return whether the handling was successful
 */
static bool handle_struct_type(const tlv_data_t *data, s_provide_ledger_account_ctx *context)
{
    UNUSED(context);
    if (!tlv_enforce_u8_value(data, TYPE_PROVIDE_LEDGER_ACCOUNT_CONTACT)) {
        PRINTF("[Provide Ledger Account] Invalid STRUCTURE_TYPE value\n");
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
static bool handle_struct_version(const tlv_data_t *data, s_provide_ledger_account_ctx *context)
{
    UNUSED(context);
    if (!tlv_enforce_u8_value(data, STRUCT_VERSION)) {
        PRINTF("[Provide Ledger Account] Invalid STRUCTURE_VERSION value\n");
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
static bool handle_contact_name(const tlv_data_t *data, s_provide_ledger_account_ctx *context)
{
    if (!address_book_handle_printable_string(data,
                                              context->ledger_account->account_name,
                                              sizeof(context->ledger_account->account_name))) {
        PRINTF("[Provide Ledger Account] CONTACT_NAME: failed to parse\n");
        return false;
    }
    return true;
}

/**
 * @brief Handler for tag \ref DERIVATION_PATH
 *
 * @param[in]  data    the tlv data
 * @param[out] context the received payload
 * @return whether the handling was successful
 */
static bool handle_derivation_path(const tlv_data_t *data, s_provide_ledger_account_ctx *context)
{
    return address_book_handle_derivation_path(data, &context->ledger_account->bip32_path);
}

/**
 * @brief Handler for tag \ref CHAIN_ID
 *
 * @param[in]  data    the tlv data
 * @param[out] context the received payload
 * @return whether the handling was successful
 */
static bool handle_chain_id(const tlv_data_t *data, s_provide_ledger_account_ctx *context)
{
    return address_book_handle_chain_id(data, &context->ledger_account->chain_id);
}

/**
 * @brief Handler for tag \ref BLOCKCHAIN_FAMILY
 *
 * @param[in]  data    the tlv data
 * @param[out] context the received payload
 * @return whether the handling was successful
 */
static bool handle_blockchain_family(const tlv_data_t *data, s_provide_ledger_account_ctx *context)
{
    return address_book_handle_blockchain_family(data, &context->ledger_account->blockchain_family);
}

/**
 * @brief Handler for tag \ref HMAC_PROOF
 *
 * @param[in]  data    the tlv data
 * @param[out] context the received payload
 * @return whether the handling was successful
 */
static bool handle_hmac_proof(const tlv_data_t *data, s_provide_ledger_account_ctx *context)
{
    buffer_t buf = {0};
    if (!get_buffer_from_tlv_data(data, &buf, CX_SHA256_SIZE, CX_SHA256_SIZE)) {
        PRINTF("[Provide Ledger Account] HMAC_PROOF: invalid length (expected %d bytes)\n",
               CX_SHA256_SIZE);
        return false;
    }
    memmove(context->hmac_proof, buf.ptr, CX_SHA256_SIZE);
    return true;
}

DEFINE_TLV_PARSER(PROVIDE_LEDGER_ACCOUNT_TAGS, NULL, provide_ledger_account_tlv_parser)

/**
 * @brief Check that all mandatory TLV tags were received
 *
 * @param[in] context the received payload
 * @return whether all mandatory fields are present
 */
static bool verify_fields(const s_provide_ledger_account_ctx *context)
{
    bool result = TLV_CHECK_RECEIVED_TAGS(context->received_tags,
                                          TAG_STRUCTURE_TYPE,
                                          TAG_STRUCTURE_VERSION,
                                          TAG_CONTACT_NAME,
                                          TAG_DERIVATION_PATH,
                                          TAG_BLOCKCHAIN_FAMILY,
                                          HMAC_PROOF);
    if (!result) {
        PRINTF("[Provide Ledger Account] Missing mandatory fields!\n");
        return false;
    }
    if (context->ledger_account->blockchain_family == FAMILY_ETHEREUM) {
        result = TLV_CHECK_RECEIVED_TAGS(context->received_tags, TAG_CHAIN_ID);
        if (!result) {
            PRINTF("[Provide Ledger Account] Missing CHAIN_ID for Ethereum family!\n");
            return false;
        }
    }
    return true;
}

/**
 * @brief Print the received descriptor
 *
 * @param[in] context the received payload
 */
static void print_payload(const s_provide_ledger_account_ctx *context)
{
    UNUSED(context);
    char out[50] = {0};

    PRINTF("****************************************************************************\n");
    PRINTF("[Provide Ledger Account] - Retrieved Descriptor:\n");
    PRINTF("[Provide Ledger Account] -    Account name:        %s\n",
           context->ledger_account->account_name);
    if (bip32_path_format_simple(&context->ledger_account->bip32_path, out, sizeof(out))) {
        PRINTF("[Provide Ledger Account] -    Derivation path[%d]: %s\n",
               context->ledger_account->bip32_path.length,
               out);
    }
    else {
        PRINTF("[Provide Ledger Account] -    Derivation path length[%d] (failed to format)\n",
               context->ledger_account->bip32_path.length);
    }
    PRINTF("[Provide Ledger Account] -    Blockchain family:   %s\n",
           FAMILY_AS_STR(context->ledger_account->blockchain_family));
    if (context->ledger_account->blockchain_family == FAMILY_ETHEREUM) {
        PRINTF("[Provide Ledger Account] -    Chain ID:            %llu\n",
               context->ledger_account->chain_id);
    }
}

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Deliver a verified Ledger Account contact to the coin application.
 *
 * Parses, verifies the HMAC Proof of Registration, and passes the account to
 * the coin app callback. Returns SWO_SUCCESS (9000) with no response data.
 *
 * @param[in] buffer_in        the fully assembled TLV payload
 * @param[in] buffer_in_length the length of the payload
 * @return bolos_err_t         the result of the command
 */
bolos_err_t provide_ledger_account_contact(uint8_t *buffer_in, size_t buffer_in_length)
{
    const buffer_t               payload = {.ptr = buffer_in, .size = buffer_in_length};
    s_provide_ledger_account_ctx ctx     = {0};

    ctx.ledger_account = &PROVIDE_LEDGER_ACCOUNT;

    // Parse using SDK TLV parser
    if (!provide_ledger_account_tlv_parser(&payload, &ctx, &ctx.received_tags)) {
        PRINTF("[Provide Ledger Account] TLV parsing failed\n");
        return SWO_INCORRECT_DATA;
    }
    if (!verify_fields(&ctx)) {
        return SWO_INCORRECT_DATA;
    }
    print_payload(&ctx);

    // Verify HMAC Proof of Registration: HMAC(bip32_path, account_name, family, chain_id)
    if (!address_book_verify_hmac_proof_ledger_account(&PROVIDE_LEDGER_ACCOUNT.bip32_path,
                                                       PROVIDE_LEDGER_ACCOUNT.account_name,
                                                       PROVIDE_LEDGER_ACCOUNT.blockchain_family,
                                                       PROVIDE_LEDGER_ACCOUNT.chain_id,
                                                       ctx.hmac_proof)) {
        PRINTF("[Provide Ledger Account] HMAC proof verification failed\n");
        return SWO_SECURITY_CONDITION_NOT_SATISFIED;
    }

    // Pass verified contact to the coin app for storage
    if (!handle_provide_ledger_account_contact(&PROVIDE_LEDGER_ACCOUNT)) {
        PRINTF("[Provide Ledger Account] Rejected by coin application\n");
        return SWO_WRONG_PARAMETER_VALUE;
    }

    return SWO_SUCCESS;
}

#endif  // HAVE_ADDRESS_BOOK && HAVE_ADDRESS_BOOK_LEDGER_ACCOUNT
