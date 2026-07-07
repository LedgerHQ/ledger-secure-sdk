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
 * @file address_book_crypto.c
 * @brief HMAC Proof of Registration for Address Book contacts
 *
 * All HMAC key derivation and computation are delegated to two OS syscalls:
 *   - sys_address_book_hmac()        — compute HMAC
 *   - sys_address_book_hmac_verify() — verify HMAC (constant-time)
 *
 * This file is responsible only for serializing the HMAC message before
 * calling the appropriate syscall.
 */

#include <string.h>
#include <stdint.h>
#include "address_book_crypto.h"
#include "ledger_account.h"
#include "identity.h"
#include "lcx_sha256.h"
#include "lcx_rng.h"
#include "os_utils.h"
#include "os_address_book.h"
#include "io.h"

#ifdef HAVE_ADDRESS_BOOK

/**
 * @brief Maximum serialized HMAC message size.
 *
 * Worst case:
 *   gid(32) + scope_len(1) + scope(32) + id_len(1) + identifier(80) + family(1) + chain_id(8)
 *   = 155 bytes
 */
#define HMAC_MSG_MAX_SIZE \
    (GID_SIZE + 1U + (SCOPE_LENGTH - 1U) + 1U + IDENTIFIER_MAX_LENGTH + 1U + 8U)

/** Buffer size for HMAC_PROOF message: gid(32) | name_len(1) | name(max 32) */
#define HMAC_PROOF_MSG_SIZE (GID_SIZE + 1U + (CONTACT_NAME_LENGTH - 1U))

/** Buffer size for Ledger Account message: name_len(1) | name(max 32) | family(1) | chain_id(8) */
#define HMAC_LEDGER_MSG_SIZE (1U + (ACCOUNT_NAME_LENGTH - 1U) + 1U + 8U)

/* Private functions ---------------------------------------------------------*/

/**
 * @brief Serialize an HMAC message from optional fields.
 *
 * Builds the message by concatenating present fields in order:
 *   1. gid (32 bytes)                         — if gid != NULL
 *   2. strlen(str1) as uint8 + str1 content   — if str1 != NULL
 *   3. raw_len as uint8 + raw bytes           — if raw != NULL
 *   4. family as uint8 [+ chain_id big-endian] — if include_family == true
 *
 * @param[out] msg             Output buffer (caller must ensure sufficient size)
 * @param[in]  msg_size        Size of @p msg in bytes (used for bounds checking)
 * @param[in]  gid             32-byte group ID, or NULL to skip
 * @param[in]  str1            Null-terminated string (length-prefixed in output), or NULL to skip
 * @param[in]  raw             Raw bytes (length-prefixed in output), or NULL to skip
 * @param[in]  raw_len         Length of @p raw in bytes (ignored if raw == NULL)
 * @param[in]  include_family  Whether to append family and optional chain_id
 * @param[in]  family          Blockchain family (used only if include_family == true)
 * @param[in]  chain_id        Chain ID, appended only for FAMILY_ETHEREUM
 *
 * @return Number of bytes written to msg, or SIZE_MAX if @p msg_size is too small
 */
static size_t serialize_hmac_msg(uint8_t            *msg,
                                 size_t              msg_size,
                                 const uint8_t      *gid,
                                 const char         *str1,
                                 const uint8_t      *raw,
                                 uint8_t             raw_len,
                                 bool                include_family,
                                 blockchain_family_e family,
                                 uint64_t            chain_id)
{
    size_t offset = 0;

    // Optional 32-byte GID prefix
    if (gid != NULL) {
        if (offset + GID_SIZE > msg_size) {
            return SIZE_MAX;
        }
        memmove(&msg[offset], gid, GID_SIZE);
        offset += GID_SIZE;
    }

    // Optional length-prefixed string
    if (str1 != NULL) {
        uint8_t len = (uint8_t) strlen(str1);
        if (offset + 1U + (size_t) len > msg_size) {
            return SIZE_MAX;
        }
        msg[offset++] = len;
        if (len > 0) {
            memmove(&msg[offset], str1, len);
            offset += len;
        }
    }

    // Optional length-prefixed raw bytes
    if (raw != NULL) {
        if (offset + 1U + (size_t) raw_len > msg_size) {
            return SIZE_MAX;
        }
        msg[offset++] = raw_len;
        memmove(&msg[offset], raw, raw_len);
        offset += raw_len;
    }

    // Optional family byte + chain_id
    if (include_family) {
        if (offset + 1U > msg_size) {
            return SIZE_MAX;
        }
        msg[offset++] = (uint8_t) family;
        if (family == FAMILY_ETHEREUM) {
            if (offset + 8U > msg_size) {
                return SIZE_MAX;
            }
            U8BE_ENCODE(msg, offset, chain_id);
            offset += 8U;
        }
    }

    return offset;
}

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Send an HMAC proof response for Edit operations
 *
 * Response format: type(1) | hmac(32)
 *
 * @param[in] type       Message type (TYPE_EDIT_CONTACT_NAME, TYPE_EDIT_IDENTIFIER, ...)
 * @param[in] hmac_proof 32-byte HMAC proof
 * @return true if sent successfully, false otherwise
 */
bool address_book_send_hmac_proof(uint8_t type, const uint8_t hmac_proof[CX_SHA256_SIZE])
{
    size_t tx = 0;

    // Response format: type(1) | hmac(32)
    G_io_tx_buffer[tx++] = type;
    memmove(&G_io_tx_buffer[tx], hmac_proof, CX_SHA256_SIZE);
    tx += CX_SHA256_SIZE;

    io_send_response_pointer(G_io_tx_buffer, tx, SWO_SUCCESS);
    return true;
}

/**
 * @brief Send the Register Identity response carrying the group handle and both HMACs
 *
 * Response format: TYPE_REGISTER_IDENTITY(1) | group_handle(64) | hmac_proof(32) | hmac_rest(32)
 * = 129 B
 *
 * @param[in] group_handle Device-authenticated group handle (gid || MAC)
 * @param[in] hmac_proof    HMAC over (gid, name)
 * @param[in] hmac_rest    HMAC over (gid, scope, identifier, family [, chain_id])
 * @return true if sent successfully, false otherwise
 */
bool address_book_send_register_identity_response(const uint8_t group_handle[GROUP_HANDLE_SIZE],
                                                  const uint8_t hmac_proof[CX_SHA256_SIZE],
                                                  const uint8_t hmac_rest[CX_SHA256_SIZE])
{
    size_t tx = 0;

    G_io_tx_buffer[tx++] = TYPE_REGISTER_IDENTITY;
    memmove(&G_io_tx_buffer[tx], group_handle, GROUP_HANDLE_SIZE);
    tx += GROUP_HANDLE_SIZE;
    memmove(&G_io_tx_buffer[tx], hmac_proof, CX_SHA256_SIZE);
    tx += CX_SHA256_SIZE;
    memmove(&G_io_tx_buffer[tx], hmac_rest, CX_SHA256_SIZE);
    tx += CX_SHA256_SIZE;

    io_send_response_pointer(G_io_tx_buffer, tx, SWO_SUCCESS);
    return true;
}

/**
 * @brief Generate a device-authenticated group handle.
 *
 * Generates a random 32-byte gid, then authenticates it via OS syscall:
 * group_handle = gid(32) || HMAC-SHA256(K_group, gid)(32)
 *
 * @param[in]  bip32_path    BIP32 path used to derive K_group
 * @param[out] group_handle  Output buffer (64 bytes): gid || MAC
 * @return true if successful, false otherwise
 */
bool address_book_generate_group_handle(const path_bip32_t *bip32_path,
                                        uint8_t             group_handle[GROUP_HANDLE_SIZE])
{
    bool     success = false;
    uint8_t *gid     = group_handle;
    uint8_t *mac     = group_handle + GID_SIZE;

    cx_rng_no_throw(gid, GID_SIZE);
    if (!sys_address_book_hmac(
            bip32_path->path, bip32_path->length, ADDRESS_BOOK_SALT_GROUP, gid, GID_SIZE, mac)) {
        goto end;
    }
    success = true;

end:
    return success;
}

/**
 * @brief Verify a group handle and extract the gid.
 *
 * Verifies MAC(K_group, gid) via OS syscall.
 * On success, copies gid into @p gid_out.
 *
 * @param[in]  bip32_path    BIP32 path used to derive K_group
 * @param[in]  group_handle  64-byte handle to verify: gid(32) || MAC(32)
 * @param[out] gid_out       Extracted gid (32 bytes), only valid on success
 * @return true if the handle is authentic, false otherwise
 */
bool address_book_verify_group_handle(const path_bip32_t *bip32_path,
                                      const uint8_t       group_handle[GROUP_HANDLE_SIZE],
                                      uint8_t             gid_out[GID_SIZE])
{
    bool           success = false;
    const uint8_t *gid     = group_handle;
    const uint8_t *mac     = group_handle + GID_SIZE;

    if (!sys_address_book_hmac_verify(
            bip32_path->path, bip32_path->length, ADDRESS_BOOK_SALT_GROUP, gid, GID_SIZE, mac)) {
        PRINTF("[Address Book] Group handle MAC verification failed\n");
        goto end;
    }
    memmove(gid_out, gid, GID_SIZE);
    success = true;

end:
    return success;
}

/**
 * @brief Compute HMAC_PROOF for an Identity contact.
 *
 * Serializes the HMAC message: gid(32) | name_len(1) | name
 * Then delegates to the OS syscall.
 *
 * @param[in]  bip32_path  BIP32 path used at registration
 * @param[in]  gid         32-byte wallet-generated contact ID
 * @param[in]  name        Contact name (null-terminated)
 * @param[out] hmac_out    Output buffer for the 32-byte HMAC
 * @return true if successful, false otherwise
 */
bool address_book_compute_hmac_proof(const path_bip32_t *bip32_path,
                                     const uint8_t       gid[GID_SIZE],
                                     const char         *name,
                                     uint8_t             hmac_out[CX_SHA256_SIZE])
{
    uint8_t msg[HMAC_PROOF_MSG_SIZE] = {0};
    bool    success                  = false;
    size_t  msg_len = serialize_hmac_msg(msg, sizeof(msg), gid, name, NULL, 0, false, 0, 0);

    if (msg_len == SIZE_MAX) {
        goto end;
    }
    if (!sys_address_book_hmac(bip32_path->path,
                               bip32_path->length,
                               ADDRESS_BOOK_SALT_IDENTITY,
                               msg,
                               msg_len,
                               hmac_out)) {
        goto end;
    }
    success = true;

end:
    explicit_bzero(msg, sizeof(msg));
    return success;
}

/**
 * @brief Verify HMAC_PROOF for an Identity contact.
 *
 * @param[in] bip32_path   BIP32 path used at registration
 * @param[in] gid          32-byte contact ID
 * @param[in] name         Contact name (null-terminated)
 * @param[in] hmac_expected 32-byte HMAC to verify against
 * @return true if valid, false otherwise
 */
bool address_book_verify_hmac_proof(const path_bip32_t *bip32_path,
                                    const uint8_t       gid[GID_SIZE],
                                    const char         *name,
                                    const uint8_t       hmac_expected[CX_SHA256_SIZE])
{
    uint8_t msg[HMAC_PROOF_MSG_SIZE] = {0};
    bool    success                  = false;
    size_t  msg_len = serialize_hmac_msg(msg, sizeof(msg), gid, name, NULL, 0, false, 0, 0);

    if (msg_len == SIZE_MAX) {
        goto end;
    }
    if (!sys_address_book_hmac_verify(bip32_path->path,
                                      bip32_path->length,
                                      ADDRESS_BOOK_SALT_IDENTITY,
                                      msg,
                                      msg_len,
                                      hmac_expected)) {
        PRINTF("HMAC_PROOF mismatch\n");
        goto end;
    }
    success = true;

end:
    explicit_bzero(msg, sizeof(msg));
    return success;
}

/**
 * @brief Compute HMAC_REST for an Identity contact.
 *
 * Serializes: gid(32) | scope_len(1) | scope | id_len(1) | identifier |
 *             family(1) [| chain_id(8) for FAMILY_ETHEREUM]
 * Then delegates to the OS syscall.
 *
 * @param[in]  bip32_path     BIP32 path used at registration
 * @param[in]  gid            32-byte contact ID
 * @param[in]  scope          Contact scope (null-terminated, may be empty)
 * @param[in]  identifier     Raw identifier bytes
 * @param[in]  identifier_len Length of identifier
 * @param[in]  family         Blockchain family
 * @param[in]  chain_id       Chain ID (ignored for non-Ethereum)
 * @param[out] hmac_out       Output buffer for the 32-byte HMAC
 * @return true if successful, false otherwise
 */
bool address_book_compute_hmac_rest(const path_bip32_t *bip32_path,
                                    const uint8_t       gid[GID_SIZE],
                                    const char         *scope,
                                    const uint8_t      *identifier,
                                    uint8_t             identifier_len,
                                    blockchain_family_e family,
                                    uint64_t            chain_id,
                                    uint8_t             hmac_out[CX_SHA256_SIZE])
{
    uint8_t msg[HMAC_MSG_MAX_SIZE] = {0};
    bool    success                = false;
    size_t  msg_len                = serialize_hmac_msg(
        msg, sizeof(msg), gid, scope, identifier, identifier_len, true, family, chain_id);

    if (msg_len == SIZE_MAX) {
        goto end;
    }
    if (!sys_address_book_hmac(bip32_path->path,
                               bip32_path->length,
                               ADDRESS_BOOK_SALT_IDENTITY,
                               msg,
                               msg_len,
                               hmac_out)) {
        goto end;
    }
    success = true;

end:
    explicit_bzero(msg, sizeof(msg));
    return success;
}

/**
 * @brief Verify HMAC_REST for an Identity contact.
 *
 * @param[in] bip32_path   BIP32 path used at registration
 * @param[in] gid          32-byte contact ID
 * @param[in] scope        Contact scope (null-terminated)
 * @param[in] identifier   Raw identifier bytes
 * @param[in] identifier_len Length of identifier
 * @param[in] family       Blockchain family
 * @param[in] chain_id     Chain ID (0 for non-Ethereum)
 * @param[in] hmac_expected 32-byte HMAC to verify against
 * @return true if valid, false otherwise
 */
bool address_book_verify_hmac_rest(const path_bip32_t *bip32_path,
                                   const uint8_t       gid[GID_SIZE],
                                   const char         *scope,
                                   const uint8_t      *identifier,
                                   uint8_t             identifier_len,
                                   blockchain_family_e family,
                                   uint64_t            chain_id,
                                   const uint8_t       hmac_expected[CX_SHA256_SIZE])
{
    uint8_t msg[HMAC_MSG_MAX_SIZE] = {0};
    bool    success                = false;
    size_t  msg_len                = serialize_hmac_msg(
        msg, sizeof(msg), gid, scope, identifier, identifier_len, true, family, chain_id);

    if (msg_len == SIZE_MAX) {
        goto end;
    }
    if (!sys_address_book_hmac_verify(bip32_path->path,
                                      bip32_path->length,
                                      ADDRESS_BOOK_SALT_IDENTITY,
                                      msg,
                                      msg_len,
                                      hmac_expected)) {
        PRINTF("HMAC_REST mismatch\n");
        goto end;
    }
    success = true;

end:
    explicit_bzero(msg, sizeof(msg));
    return success;
}

#ifdef HAVE_ADDRESS_BOOK_LEDGER_ACCOUNT

/**
 * @brief Compute an HMAC-SHA256 Proof of Registration for a Ledger Account.
 *
 * Serializes: name_len(1) | name | family(1) [| chain_id(8)]
 * Then delegates to the OS syscall.
 *
 * @param[in]  bip32_path  BIP32 path used at registration
 * @param[in]  name        Account name (null-terminated)
 * @param[in]  family      Blockchain family
 * @param[in]  chain_id    Chain ID (ignored for non-Ethereum families)
 * @param[out] hmac_out    Output buffer for the 32-byte HMAC proof
 * @return true if successful, false otherwise
 */
bool address_book_compute_hmac_proof_ledger_account(const path_bip32_t *bip32_path,
                                                    const char         *name,
                                                    blockchain_family_e family,
                                                    uint64_t            chain_id,
                                                    uint8_t             hmac_out[CX_SHA256_SIZE])
{
    uint8_t msg[HMAC_LEDGER_MSG_SIZE] = {0};
    bool    success                   = false;
    size_t  msg_len
        = serialize_hmac_msg(msg, sizeof(msg), NULL, name, NULL, 0, true, family, chain_id);

    if (msg_len == SIZE_MAX) {
        goto end;
    }
    if (!sys_address_book_hmac(bip32_path->path,
                               bip32_path->length,
                               ADDRESS_BOOK_SALT_LEDGER_ACCOUNT,
                               msg,
                               msg_len,
                               hmac_out)) {
        goto end;
    }
    success = true;

end:
    explicit_bzero(msg, sizeof(msg));
    return success;
}

/**
 * @brief Verify an HMAC Proof of Registration for a Ledger Account.
 *
 * @param[in] bip32_path   BIP32 path used at registration
 * @param[in] name         Account name (null-terminated)
 * @param[in] family       Blockchain family
 * @param[in] chain_id     Chain ID (0 for non-Ethereum)
 * @param[in] hmac_expected 32-byte HMAC proof to verify against
 * @return true if the proof is valid, false otherwise
 */
bool address_book_verify_hmac_proof_ledger_account(const path_bip32_t *bip32_path,
                                                   const char         *name,
                                                   blockchain_family_e family,
                                                   uint64_t            chain_id,
                                                   const uint8_t hmac_expected[CX_SHA256_SIZE])
{
    uint8_t msg[HMAC_LEDGER_MSG_SIZE] = {0};
    bool    success                   = false;
    size_t  msg_len
        = serialize_hmac_msg(msg, sizeof(msg), NULL, name, NULL, 0, true, family, chain_id);

    if (msg_len == SIZE_MAX) {
        goto end;
    }
    if (!sys_address_book_hmac_verify(bip32_path->path,
                                      bip32_path->length,
                                      ADDRESS_BOOK_SALT_LEDGER_ACCOUNT,
                                      msg,
                                      msg_len,
                                      hmac_expected)) {
        PRINTF("HMAC proof mismatch\n");
        goto end;
    }
    success = true;

end:
    explicit_bzero(msg, sizeof(msg));
    return success;
}

#endif  // HAVE_ADDRESS_BOOK_LEDGER_ACCOUNT
#endif  // HAVE_ADDRESS_BOOK
