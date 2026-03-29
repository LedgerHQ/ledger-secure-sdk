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
 * @file verify_address.c
 * @brief Verify Signed Address flow (V2 - "Contacts" proposal)
 *
 * Flow:
 *  1. Parse TLV payload
 *  2. Verify Ledger PKI signature (authenticates the Ledger backend payload)
 *  3. Verify HMAC Proof of Registration: re-derives and compares the HMAC.
 *     Proves the contact (name + identity_pubkey) was registered on this device.
 *  4. Verify ECDSA signature of address_raw by identity_pubkey.
 *     Proves the intended receiver produced this address with their device.
 *  5. Coin-app validation: handle_check_verified_address()
 *  6. Display: contact name + address (user sees a name, not raw bytes)
 *  7. On confirm: send SWO_SUCCESS (no encrypted payload needed)
 *
 * TLV structure type: TYPE_VERIFY_ADDRESS (0x16)
 */

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "lcx_ecdsa.h"
#include "lcx_sha256.h"
#include "lcx_ecfp.h"
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
#include "address_book_common.h"
#include "contact.h"
#include "verify_address.h"
#include "io.h"
#include "nbgl_use_case.h"

#if defined(HAVE_ADDRESS_BOOK) && defined(HAVE_ADDRESS_BOOK_CONTACTS)

/* Private defines------------------------------------------------------------*/
#define STRUCT_VERSION 0x01

// New TLV tags for V2
// Note: 0x24=TICKER, 0x27=TX_HASH, 0x28=DOMAIN_HASH are reserved in other structures.
#define TAG_IDENTITY_PUBKEY_VALUE   0x25
#define TAG_HMAC_PROOF_VALUE        0x26
#define TAG_ADDRESS_SIGNATURE_VALUE 0x29

/* Private types, structures, unions -----------------------------------------*/
typedef struct {
    verify_address_t *verify_address;
    uint8_t           sig_size;
    const uint8_t    *sig;
    cx_sha256_t       hash_ctx;
    TLV_reception_t   received_tags;
} s_verify_addr_ctx;

/* Private macros-------------------------------------------------------------*/
#define VERIFY_ADDRESS_TAGS(X)                                                   \
    X(0x01, TAG_STRUCTURE_TYPE, handle_struct_type, ENFORCE_UNIQUE_TAG)          \
    X(0x02, TAG_STRUCTURE_VERSION, handle_struct_version, ENFORCE_UNIQUE_TAG)    \
    X(0x0A, TAG_CONTACT_NAME, handle_contact_name, ENFORCE_UNIQUE_TAG)           \
    X(0x25, TAG_IDENTITY_PUBKEY, handle_identity_pubkey, ENFORCE_UNIQUE_TAG)     \
    X(0x26, TAG_HMAC_PROOF, handle_hmac_proof, ENFORCE_UNIQUE_TAG)               \
    X(0x22, TAG_ADDRESS_RAW, handle_address_raw, ENFORCE_UNIQUE_TAG)             \
    X(0x29, TAG_ADDRESS_SIGNATURE, handle_address_signature, ENFORCE_UNIQUE_TAG) \
    X(0x21, TAG_DERIVATION_PATH, handle_derivation_path, ENFORCE_UNIQUE_TAG)     \
    X(0x51, TAG_BLOCKCHAIN_FAMILY, handle_blockchain_family, ENFORCE_UNIQUE_TAG) \
    X(0x23, TAG_CHAIN_ID, handle_chain_id, ENFORCE_UNIQUE_TAG)                   \
    X(0x15, TAG_DER_SIGNATURE, handle_signature, ENFORCE_UNIQUE_TAG)

/* Private functions prototypes ----------------------------------------------*/
static bool common_handler(const tlv_data_t *data, s_verify_addr_ctx *context);

/* Private variables ---------------------------------------------------------*/
static verify_address_t           VERIFY_ADDR  = {0};
static nbgl_contentTagValueList_t ui_pairsList = {0};

/* Private functions ---------------------------------------------------------*/

static bool handle_struct_type(const tlv_data_t *data, s_verify_addr_ctx *context)
{
    UNUSED(context);
    return tlv_check_structure_type(data, TYPE_VERIFY_ADDRESS);
}

static bool handle_struct_version(const tlv_data_t *data, s_verify_addr_ctx *context)
{
    UNUSED(context);
    return tlv_check_structure_version(data, STRUCT_VERSION);
}

static bool handle_contact_name(const tlv_data_t *data, s_verify_addr_ctx *context)
{
    if (!address_book_handle_printable_string(data,
                                              (char *) context->verify_address->contact_name,
                                              sizeof(context->verify_address->contact_name))) {
        PRINTF("CONTACT_NAME: failed to parse\n");
        return false;
    }
    return true;
}

static bool handle_identity_pubkey(const tlv_data_t *data, s_verify_addr_ctx *context)
{
    buffer_t buf = {0};
    if (!get_buffer_from_tlv_data(data, &buf, IDENTITY_PUBKEY_LENGTH, IDENTITY_PUBKEY_LENGTH)) {
        PRINTF("IDENTITY_PUBKEY: expected %d bytes\n", IDENTITY_PUBKEY_LENGTH);
        return false;
    }
    if (buf.ptr[0] != 0x02 && buf.ptr[0] != 0x03) {
        PRINTF("IDENTITY_PUBKEY: not a compressed secp256r1 key\n");
        return false;
    }
    memmove(context->verify_address->identity_pubkey, buf.ptr, IDENTITY_PUBKEY_LENGTH);
    return true;
}

static bool handle_hmac_proof(const tlv_data_t *data, s_verify_addr_ctx *context)
{
    buffer_t buf = {0};
    if (!get_buffer_from_tlv_data(data, &buf, HMAC_PROOF_LENGTH, HMAC_PROOF_LENGTH)) {
        PRINTF("HMAC_PROOF: expected %d bytes\n", HMAC_PROOF_LENGTH);
        return false;
    }
    memmove(context->verify_address->hmac_proof, buf.ptr, HMAC_PROOF_LENGTH);
    return true;
}

static bool handle_address_raw(const tlv_data_t *data, s_verify_addr_ctx *context)
{
    buffer_t address = {0};
    if (!get_buffer_from_tlv_data(data, &address, 1, MAX_ADDRESS_LENGTH)) {
        PRINTF("ADDRESS_RAW: failed to extract\n");
        return false;
    }
    if (is_zeroes_buffer(address.ptr, address.size)) {
        PRINTF("ADDRESS_RAW: all zeroes\n");
        return false;
    }
    memmove(context->verify_address->address_raw, address.ptr, address.size);
    context->verify_address->address_length = (uint8_t) address.size;
    return true;
}

static bool handle_address_signature(const tlv_data_t *data, s_verify_addr_ctx *context)
{
    buffer_t buf = {0};
    if (!get_buffer_from_tlv_data(
            data, &buf, CX_ECDSA_SHA256_SIG_MIN_ASN1_LENGTH, MAX_IDENTITY_SIG_LENGTH)) {
        PRINTF("ADDRESS_SIGNATURE: failed to extract\n");
        return false;
    }
    memmove(context->verify_address->address_sig, buf.ptr, buf.size);
    context->verify_address->address_sig_len = (uint8_t) buf.size;
    return true;
}

static bool handle_derivation_path(const tlv_data_t *data, s_verify_addr_ctx *context)
{
    return address_book_handle_derivation_path(data, &context->verify_address->bip32_path);
}

static bool handle_blockchain_family(const tlv_data_t *data, s_verify_addr_ctx *context)
{
    return address_book_handle_blockchain_family(data, &context->verify_address->blockchain_family);
}

static bool handle_chain_id(const tlv_data_t *data, s_verify_addr_ctx *context)
{
    return address_book_handle_chain_id(data, &context->verify_address->chain_id);
}

static bool handle_signature(const tlv_data_t *data, s_verify_addr_ctx *context)
{
    return address_book_handle_signature(data, &context->sig, &context->sig_size);
}

DEFINE_TLV_PARSER(VERIFY_ADDRESS_TAGS, &common_handler, verify_addr_tlv_parser)

static bool common_handler(const tlv_data_t *data, s_verify_addr_ctx *context)
{
    return address_book_hash_handler(data, &context->hash_ctx);
}

static bool verify_fields(const s_verify_addr_ctx *context)
{
    bool result = TLV_CHECK_RECEIVED_TAGS(context->received_tags,
                                          TAG_STRUCTURE_TYPE,
                                          TAG_STRUCTURE_VERSION,
                                          TAG_CONTACT_NAME,
                                          TAG_IDENTITY_PUBKEY,
                                          TAG_HMAC_PROOF,
                                          TAG_ADDRESS_RAW,
                                          TAG_ADDRESS_SIGNATURE,
                                          TAG_DERIVATION_PATH,
                                          TAG_BLOCKCHAIN_FAMILY,
                                          TAG_DER_SIGNATURE);
    if (!result) {
        PRINTF("Missing mandatory fields in Verify Address descriptor!\n");
        return false;
    }
    if (context->verify_address->blockchain_family == FAMILY_ETHEREUM) {
        if (!TLV_CHECK_RECEIVED_TAGS(context->received_tags, TAG_CHAIN_ID)) {
            PRINTF("Missing TAG_CHAIN_ID for Ethereum!\n");
            return false;
        }
    }
    return true;
}

/**
 * @brief Verify the ECDSA signature of address_raw by identity_pubkey.
 *
 * The expected message is: SHA256(address_raw).
 * The signature is DER-encoded ECDSA over secp256r1.
 */
static bool verify_address_signature(const verify_address_t *va)
{
    cx_ecfp_256_public_key_t pubkey               = {0};
    uint8_t                  hash[CX_SHA256_SIZE] = {0};
    cx_err_t                 error                = CX_INTERNAL_ERROR;
    bool                     success              = false;

    // Load the identity public key (compressed secp256r1)
    CX_CHECK(cx_ecfp_init_public_key_no_throw(
        CX_CURVE_SECP256R1, va->identity_pubkey, IDENTITY_PUBKEY_LENGTH, &pubkey));

    // Hash the address
    CX_CHECK(cx_hash_sha256(va->address_raw, va->address_length, hash, sizeof(hash)));

    // Verify the ECDSA signature
    if (!cx_ecdsa_verify_no_throw(
            &pubkey, hash, sizeof(hash), va->address_sig, va->address_sig_len)) {
        PRINTF("Address signature verification failed\n");
        goto end;
    }
    success = true;

end:
    explicit_bzero(&pubkey, sizeof(pubkey));
    explicit_bzero(hash, sizeof(hash));
    return success;
}

static bool verify_struct(const s_verify_addr_ctx *context)
{
    const verify_address_t *va = context->verify_address;

    if (!verify_fields(context)) {
        return false;
    }

    // Step 1: Verify Ledger PKI signature (authenticates the payload origin)
    if (!address_book_verify_signature(
            (cx_sha256_t *) &context->hash_ctx, context->sig, context->sig_size)) {
        PRINTF("PKI signature verification failed for Verify Address!\n");
        return false;
    }

    // Step 2: Verify HMAC Proof of Registration
    // Proves this (name, identity_pubkey) pair was registered on this device.
    if (!address_book_verify_hmac_proof(
            &va->bip32_path, va->contact_name, va->identity_pubkey, va->hmac_proof)) {
        PRINTF("HMAC proof verification failed: contact not registered on this device!\n");
        return false;
    }

    // Step 3: Verify address is signed by the contact's identity key
    // Proves the intended receiver produced this address.
    if (!verify_address_signature(va)) {
        PRINTF("Address signature verification failed!\n");
        return false;
    }

    PRINTF("[Verify Address] contact=%s addr_len=%d\n", va->contact_name, va->address_length);
    return true;
}

static void review_verify_address_choice(bool confirm)
{
    if (confirm) {
        io_send_sw(SWO_SUCCESS);
        nbgl_useCaseStatus("Address verified", true, finalize_ui_verify_address);
    }
    else {
        io_send_sw(SWO_INCORRECT_DATA);
        nbgl_useCaseReviewStatus(STATUS_TYPE_OPERATION_REJECTED, finalize_ui_verify_address);
    }
}

static void ui_display_verify_address(void)
{
    memset(&ui_pairsList, 0, sizeof(ui_pairsList));
    ui_pairsList.nbPairs  = 2;  // contact name + address
    ui_pairsList.callback = get_verify_address_tagValue;
    ui_pairsList.wrapping = true;

    nbgl_useCaseReviewLight(TYPE_OPERATION,
                            &ui_pairsList,
                            &LARGE_ADDRESS_BOOK_ICON,
                            "Verify address",
                            NULL,
                            "Confirm address belongs to contact?",
                            review_verify_address_choice);
}

/* Exported functions --------------------------------------------------------*/

bolos_err_t verify_signed_address(uint8_t *buffer_in, size_t buffer_in_length)
{
    const buffer_t    payload = {.ptr = buffer_in, .size = buffer_in_length};
    s_verify_addr_ctx ctx     = {0};

    ctx.verify_address = &VERIFY_ADDR;
    cx_sha256_init(&ctx.hash_ctx);

    if (!verify_addr_tlv_parser(&payload, &ctx, &ctx.received_tags)) {
        return SWO_INCORRECT_DATA;
    }
    if (!verify_struct(&ctx)) {
        return SWO_SECURITY_CONDITION_NOT_SATISFIED;
    }
    if (!handle_check_verified_address(ctx.verify_address)) {
        PRINTF("Error: Verified address rejected by coin application\n");
        return SWO_WRONG_PARAMETER_VALUE;
    }

    ui_display_verify_address();
    return SWO_NO_RESPONSE;
}

#endif  // HAVE_ADDRESS_BOOK && HAVE_ADDRESS_BOOK_CONTACTS
