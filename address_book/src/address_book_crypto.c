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
 * Provides HMAC-SHA256 based proofs that bind a registered contact to this
 * specific device. Two independent key derivations are used, one per feature:
 *
 *  - Identity KDF:       SHA256("AddressBook-Identity"      || privkey.d)
 *  - Ledger Account KDF: SHA256("AddressBook-LedgerAccount" || privkey.d)
 *
 * HMAC messages:
 *  - Group KDF:          SHA256("AddressBook-Group"         || privkey.d)
 *  - Identity HMAC_NAME: gid(32) | name_len(1) | name
 *  - Identity HMAC_REST: gid(32) | scope_len(1) | scope | id_len(1) | identifier |
 *                        family(1) [| chain_id(8) for FAMILY_ETHEREUM]
 *  - Ledger Account:     name_len(1) | name | family(1) [| chain_id(8) for FAMILY_ETHEREUM]
 *
 * The proof is stored by the host and re-verified during Rename operations to
 * confirm that the contact was registered on this device.
 */

#include <string.h>
#include "address_book_crypto.h"
#include "ledger_account.h"
#include "identity.h"
#include "lcx_sha256.h"
#include "lcx_hmac.h"
#include "lcx_rng.h"
#include "os_utils.h"
#include "io.h"
#include "crypto_helpers.h"

#ifdef HAVE_ADDRESS_BOOK

/* Private functions ---------------------------------------------------------*/

/**
 * @brief Derive HMAC-SHA256 key from secp256r1 private key.
 *
 * KDF: SHA256(salt || privkey.d)
 *
 * @param[in]  privkey  secp256r1 private key
 * @param[in]  salt     Salt string (without null terminator)
 * @param[in]  salt_len Salt length
 * @param[out] hmac_key Derived HMAC key (32 bytes)
 * @return true if successful, false otherwise
 */
static bool address_book_derive_hmac_key(const cx_ecfp_256_private_key_t *privkey,
                                         const uint8_t                   *salt,
                                         size_t                           salt_len,
                                         uint8_t                          hmac_key[CX_SHA256_SIZE])
{
    cx_sha256_t hash_ctx             = {0};
    uint8_t     hash[CX_SHA256_SIZE] = {0};
    cx_err_t    error                = CX_INTERNAL_ERROR;
    bool        success              = false;

    cx_sha256_init(&hash_ctx);
    CX_CHECK(cx_hash_no_throw((cx_hash_t *) &hash_ctx, 0, salt, salt_len, NULL, 0));
    CX_CHECK(cx_hash_no_throw(
        (cx_hash_t *) &hash_ctx, CX_LAST, privkey->d, privkey->d_len, hash, sizeof(hash)));
    memmove(hmac_key, hash, CX_SHA256_SIZE);
    success = true;

end:
    explicit_bzero(hash, sizeof(hash));
    return success;
}

/**
 * @brief Initialise a BIP32-derived HMAC-SHA256 context.
 *
 * Derives the secp256r1 private key from @p bip32_path, runs the KDF
 * (SHA256(salt || privkey.d)), initialises @p hmac_ctx, then zeroes the
 * intermediate key material before returning.
 *
 * @param[in]  bip32_path  BIP32 path
 * @param[in]  salt        KDF salt (without null terminator)
 * @param[in]  salt_len    Salt length
 * @param[out] hmac_ctx    Initialised HMAC-SHA256 context
 * @return true if successful, false otherwise
 */
static bool init_hmac_ctx(const path_bip32_t *bip32_path,
                          const uint8_t      *salt,
                          size_t              salt_len,
                          cx_hmac_sha256_t   *hmac_ctx)
{
    cx_ecfp_256_private_key_t private_key              = {0};
    uint8_t                   hmac_key[CX_SHA256_SIZE] = {0};
    cx_err_t                  error                    = CX_INTERNAL_ERROR;
    bool                      success                  = false;

    CX_CHECK(bip32_derive_init_privkey_256(
        CX_CURVE_SECP256R1, bip32_path->path, bip32_path->length, &private_key, NULL));
    if (!address_book_derive_hmac_key(&private_key, salt, salt_len, hmac_key)) {
        goto end;
    }
    CX_CHECK(cx_hmac_sha256_init_no_throw(hmac_ctx, hmac_key, CX_SHA256_SIZE));
    success = true;

end:
    explicit_bzero(&private_key, sizeof(private_key));
    explicit_bzero(hmac_key, sizeof(hmac_key));
    return success;
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
 * Response format: TYPE_REGISTER_IDENTITY(1) | group_handle(64) | hmac_name(32) | hmac_rest(32)
 * = 129 B
 *
 * @param[in] group_handle Device-authenticated group handle (gid || MAC)
 * @param[in] hmac_name    HMAC over (gid, name)
 * @param[in] hmac_rest    HMAC over (gid, scope, identifier, family [, chain_id])
 * @return true if sent successfully, false otherwise
 */
bool address_book_send_register_identity_response(const uint8_t group_handle[GROUP_HANDLE_SIZE],
                                                  const uint8_t hmac_name[CX_SHA256_SIZE],
                                                  const uint8_t hmac_rest[CX_SHA256_SIZE])
{
    size_t tx = 0;

    G_io_tx_buffer[tx++] = TYPE_REGISTER_IDENTITY;
    memmove(&G_io_tx_buffer[tx], group_handle, GROUP_HANDLE_SIZE);
    tx += GROUP_HANDLE_SIZE;
    memmove(&G_io_tx_buffer[tx], hmac_name, CX_SHA256_SIZE);
    tx += CX_SHA256_SIZE;
    memmove(&G_io_tx_buffer[tx], hmac_rest, CX_SHA256_SIZE);
    tx += CX_SHA256_SIZE;

    io_send_response_pointer(G_io_tx_buffer, tx, SWO_SUCCESS);
    return true;
}

/**
 * @brief Generate a device-authenticated group handle.
 *
 * Generates a random 32-byte gid, then authenticates it with a device-derived MAC:
 * group_handle = gid(32) || HMAC-SHA256(K_group, gid)(32)
 *
 * K_group = SHA256("AddressBook-Group" || privkey.d)
 *
 * @param[in]  bip32_path    BIP32 path used to derive K_group
 * @param[out] group_handle  Output buffer (64 bytes): gid || MAC
 * @return true if successful, false otherwise
 */
bool address_book_generate_group_handle(const path_bip32_t *bip32_path,
                                        uint8_t             group_handle[GROUP_HANDLE_SIZE])
{
    static const uint8_t salt[]   = "AddressBook-Group";
    cx_hmac_sha256_t     hmac_ctx = {0};
    cx_err_t             error    = CX_INTERNAL_ERROR;
    bool                 success  = false;
    uint8_t             *gid      = group_handle;
    uint8_t             *mac      = group_handle + GID_SIZE;

    cx_rng_no_throw(gid, GID_SIZE);
    if (!init_hmac_ctx(bip32_path, salt, sizeof(salt) - 1, &hmac_ctx)) {
        goto end;
    }
    CX_CHECK(
        cx_hmac_no_throw((cx_hmac_t *) &hmac_ctx, CX_LAST, gid, GID_SIZE, mac, CX_SHA256_SIZE));
    success = true;

end:
    return success;
}

/**
 * @brief Verify a group handle and extract the gid.
 *
 * Recomputes MAC(K_group, gid) and compares with the embedded MAC.
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
    static const uint8_t salt[]                       = "AddressBook-Group";
    cx_hmac_sha256_t     hmac_ctx                     = {0};
    uint8_t              mac_expected[CX_SHA256_SIZE] = {0};
    cx_err_t             error                        = CX_INTERNAL_ERROR;
    bool                 success                      = false;
    const uint8_t       *gid                          = group_handle;
    const uint8_t       *mac                          = group_handle + GID_SIZE;

    if (!init_hmac_ctx(bip32_path, salt, sizeof(salt) - 1, &hmac_ctx)) {
        goto end;
    }
    CX_CHECK(cx_hmac_no_throw(
        (cx_hmac_t *) &hmac_ctx, CX_LAST, gid, GID_SIZE, mac_expected, CX_SHA256_SIZE));
    if (os_secure_memcmp(mac, mac_expected, CX_SHA256_SIZE) != 0) {
        PRINTF("[Address Book] Group handle MAC verification failed\n");
        goto end;
    }
    memmove(gid_out, gid, GID_SIZE);
    success = true;

end:
    explicit_bzero(mac_expected, sizeof(mac_expected));
    return success;
}

/**
 * @brief Compute HMAC_NAME for an Identity contact.
 *
 * HMAC key: SHA256("AddressBook-Identity" || privkey.d)
 * HMAC message: gid(32) | name_len(1) | name
 *
 * @param[in]  bip32_path  BIP32 path used at registration
 * @param[in]  gid         32-byte wallet-generated contact ID
 * @param[in]  name        Contact name (null-terminated)
 * @param[out] hmac_out    Output buffer for the 32-byte HMAC
 * @return true if successful, false otherwise
 */
bool address_book_compute_hmac_name(const path_bip32_t *bip32_path,
                                    const uint8_t       gid[GID_SIZE],
                                    const char         *name,
                                    uint8_t             hmac_out[CX_SHA256_SIZE])
{
    static const uint8_t salt[]   = "AddressBook-Identity";
    cx_hmac_sha256_t     hmac_ctx = {0};
    cx_err_t             error    = CX_INTERNAL_ERROR;
    bool                 success  = false;
    uint8_t              name_len = (uint8_t) strlen(name);

    if (!init_hmac_ctx(bip32_path, salt, sizeof(salt) - 1, &hmac_ctx)) {
        goto end;
    }
    // gid(16)
    CX_CHECK(cx_hmac_no_throw((cx_hmac_t *) &hmac_ctx, 0, gid, GID_SIZE, NULL, 0));
    // name_len(1) | name
    CX_CHECK(cx_hmac_no_throw((cx_hmac_t *) &hmac_ctx, 0, &name_len, 1, NULL, 0));
    CX_CHECK(cx_hmac_no_throw((cx_hmac_t *) &hmac_ctx,
                              CX_LAST,
                              (const uint8_t *) name,
                              name_len,
                              hmac_out,
                              CX_SHA256_SIZE));
    success = true;

end:
    return success;
}

/**
 * @brief Verify HMAC_NAME for an Identity contact.
 *
 * @param[in] bip32_path   BIP32 path used at registration
 * @param[in] gid   16-byte contact ID
 * @param[in] name         Contact name (null-terminated)
 * @param[in] hmac_expected 32-byte HMAC to verify against
 * @return true if valid, false otherwise
 */
bool address_book_verify_hmac_name(const path_bip32_t *bip32_path,
                                   const uint8_t       gid[GID_SIZE],
                                   const char         *name,
                                   const uint8_t       hmac_expected[CX_SHA256_SIZE])
{
    uint8_t hmac_computed[CX_SHA256_SIZE] = {0};
    bool    success                       = false;

    if (!address_book_compute_hmac_name(bip32_path, gid, name, hmac_computed)) {
        goto end;
    }
    if (os_secure_memcmp(hmac_computed, hmac_expected, CX_SHA256_SIZE) != 0) {
        PRINTF("HMAC_NAME mismatch\n");
        goto end;
    }
    success = true;

end:
    explicit_bzero(hmac_computed, sizeof(hmac_computed));
    return success;
}

/**
 * @brief Compute HMAC_REST for an Identity contact.
 *
 * HMAC key: SHA256("AddressBook-Identity" || privkey.d)
 * HMAC message: gid(16) | scope_len(1) | scope | id_len(1) | identifier |
 *               family(1) [| chain_id(8) for FAMILY_ETHEREUM]
 *
 * @param[in]  bip32_path     BIP32 path used at registration
 * @param[in]  gid     16-byte contact ID
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
    static const uint8_t salt[]      = "AddressBook-Identity";
    cx_hmac_sha256_t     hmac_ctx    = {0};
    cx_err_t             error       = CX_INTERNAL_ERROR;
    bool                 success     = false;
    uint8_t              scope_len   = (scope != NULL) ? (uint8_t) strlen(scope) : 0;
    uint8_t              family_byte = (uint8_t) family;

    if (!init_hmac_ctx(bip32_path, salt, sizeof(salt) - 1, &hmac_ctx)) {
        goto end;
    }
    // gid(16)
    CX_CHECK(cx_hmac_no_throw((cx_hmac_t *) &hmac_ctx, 0, gid, GID_SIZE, NULL, 0));
    // scope_len(1) | scope
    CX_CHECK(cx_hmac_no_throw((cx_hmac_t *) &hmac_ctx, 0, &scope_len, 1, NULL, 0));
    if (scope_len > 0) {
        CX_CHECK(cx_hmac_no_throw(
            (cx_hmac_t *) &hmac_ctx, 0, (const uint8_t *) scope, scope_len, NULL, 0));
    }
    // id_len(1) | identifier
    CX_CHECK(cx_hmac_no_throw((cx_hmac_t *) &hmac_ctx, 0, &identifier_len, 1, NULL, 0));
    CX_CHECK(cx_hmac_no_throw((cx_hmac_t *) &hmac_ctx, 0, identifier, identifier_len, NULL, 0));
    // family(1)
    CX_CHECK(cx_hmac_no_throw((cx_hmac_t *) &hmac_ctx, 0, &family_byte, 1, NULL, 0));
    // chain_id(8) — Ethereum only
    if (family == FAMILY_ETHEREUM) {
        uint8_t chain_id_be[8] = {0};
        U8BE_ENCODE(chain_id_be, 0, chain_id);
        CX_CHECK(cx_hmac_no_throw(
            (cx_hmac_t *) &hmac_ctx, 0, chain_id_be, sizeof(chain_id_be), NULL, 0));
    }
    CX_CHECK(cx_hmac_no_throw((cx_hmac_t *) &hmac_ctx, CX_LAST, NULL, 0, hmac_out, CX_SHA256_SIZE));
    success = true;

end:
    return success;
}

/**
 * @brief Verify HMAC_REST for an Identity contact.
 *
 * @param[in] bip32_path   BIP32 path used at registration
 * @param[in] gid   16-byte contact ID
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
    uint8_t hmac_computed[CX_SHA256_SIZE] = {0};
    bool    success                       = false;

    if (!address_book_compute_hmac_rest(
            bip32_path, gid, scope, identifier, identifier_len, family, chain_id, hmac_computed)) {
        goto end;
    }
    if (os_secure_memcmp(hmac_computed, hmac_expected, CX_SHA256_SIZE) != 0) {
        PRINTF("HMAC_REST mismatch\n");
        goto end;
    }
    success = true;

end:
    explicit_bzero(hmac_computed, sizeof(hmac_computed));
    return success;
}

#ifdef HAVE_ADDRESS_BOOK_LEDGER_ACCOUNT

/**
 * @brief Compute an HMAC-SHA256 Proof of Registration for a Ledger Account.
 *
 * HMAC key: SHA256("AddressBook-LedgerAccount" || privkey.d)
 * HMAC message: name_len(1) | name | family(1) [| chain_id(8)]
 *
 * chain_id(8) is included only when family == FAMILY_ETHEREUM.
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
    static const uint8_t salt[]      = "AddressBook-LedgerAccount";
    cx_hmac_sha256_t     hmac_ctx    = {0};
    cx_err_t             error       = CX_INTERNAL_ERROR;
    bool                 success     = false;
    uint8_t              name_len    = (uint8_t) strlen(name);
    uint8_t              family_byte = (uint8_t) family;

    if (!init_hmac_ctx(bip32_path, salt, sizeof(salt) - 1, &hmac_ctx)) {
        goto end;
    }
    // name_len(1) | name
    CX_CHECK(cx_hmac_no_throw((cx_hmac_t *) &hmac_ctx, 0, &name_len, 1, NULL, 0));
    CX_CHECK(
        cx_hmac_no_throw((cx_hmac_t *) &hmac_ctx, 0, (const uint8_t *) name, name_len, NULL, 0));
    // family(1)
    CX_CHECK(cx_hmac_no_throw((cx_hmac_t *) &hmac_ctx, 0, &family_byte, 1, NULL, 0));
    // chain_id(8) — Ethereum only
    if (family == FAMILY_ETHEREUM) {
        uint8_t chain_id_be[8] = {0};
        U8BE_ENCODE(chain_id_be, 0, chain_id);
        CX_CHECK(cx_hmac_no_throw(
            (cx_hmac_t *) &hmac_ctx, 0, chain_id_be, sizeof(chain_id_be), NULL, 0));
    }
    CX_CHECK(cx_hmac_no_throw((cx_hmac_t *) &hmac_ctx, CX_LAST, NULL, 0, hmac_out, CX_SHA256_SIZE));
    success = true;

end:
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
    uint8_t hmac_computed[CX_SHA256_SIZE] = {0};
    bool    success                       = false;

    if (!address_book_compute_hmac_proof_ledger_account(
            bip32_path, name, family, chain_id, hmac_computed)) {
        goto end;
    }
    if (os_secure_memcmp(hmac_computed, hmac_expected, CX_SHA256_SIZE) != 0) {
        PRINTF("HMAC proof mismatch\n");
        goto end;
    }
    success = true;

end:
    explicit_bzero(hmac_computed, sizeof(hmac_computed));
    return success;
}

#endif  // HAVE_ADDRESS_BOOK_LEDGER_ACCOUNT
#endif  // HAVE_ADDRESS_BOOK
