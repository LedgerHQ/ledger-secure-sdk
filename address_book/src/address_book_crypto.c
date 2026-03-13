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
#include <string.h>
#include "address_book_crypto.h"
#include "external_address.h"
#include "ledger_account.h"
#include "lcx_sha256.h"
#include "lcx_aes_gcm.h"
#include "os_random.h"
#include "os_utils.h"
#include "io.h"
#include "crypto_helpers.h"

#ifdef HAVE_ADDRESS_BOOK

/* Constants -----------------------------------------------------------------*/

// AES-256 key size and IV size for GCM
#define AES_GCM_IV_SIZE  12
#define AES_GCM_TAG_SIZE 16

// Minimum encrypted block size: len(1) + ciphertext(≥1) + len(1) + iv(12) + len(1) + tag(16)
#define MIN_ENCRYPTED_BLOCK_SIZE (1 + 1 + 1 + AES_GCM_IV_SIZE + 1 + AES_GCM_TAG_SIZE)

// Maximum remaining payload size calculation
#define MAX_REMAINING_PAYLOAD MAX(MAX_EXTERNAL_REMAINING, MAX_LEDGER_REMAINING)

// Maximum name length (both types use same max)
#define MAX_NAME_LENGTH MAX(EXTERNAL_NAME_LENGTH, ACCOUNT_NAME_LENGTH)

// Maximum plaintext length (no signature anymore, just the payload)
#define MAX_PLAINTEXT_LENGTH MAX(MAX_REMAINING_PAYLOAD, MAX_NAME_LENGTH)

/* Helper Functions ----------------------------------------------------------*/

/**
 * @brief Derive AES-256 encryption key from secp256k1 private key using KDF
 *
 * Simple KDF: SHA256(salt || privkey)
 *
 * @param[in] privkey secp256k1 private key
 * @param[out] aes_key Derived AES-256 key (32 bytes)
 * @return true if successful, false otherwise
 */
static bool address_book_derive_aes_key(const cx_ecfp_256_private_key_t *privkey,
                                        uint8_t aes_key[CX_AES_256_KEY_LEN])
{
    cx_sha256_t   hash_ctx             = {0};
    uint8_t       hash[CX_SHA256_SIZE] = {0};
    const uint8_t salt[]               = "AddressBook-AES-256-Key";
    cx_err_t      error                = CX_INTERNAL_ERROR;
    bool          success              = false;

    // Simple KDF: SHA256(salt || privkey)
    cx_sha256_init(&hash_ctx);
    CX_CHECK(cx_hash_no_throw((cx_hash_t *) &hash_ctx, 0, salt, sizeof(salt) - 1, NULL, 0));
    CX_CHECK(cx_hash_no_throw(
        (cx_hash_t *) &hash_ctx, CX_LAST, privkey->d, privkey->d_len, hash, sizeof(hash)));

    // Use the full SHA256 output as AES-256 key
    memmove(aes_key, hash, CX_AES_256_KEY_LEN);
    success = true;

end:
    explicit_bzero(hash, sizeof(hash));
    return success;
}

/**
 * @brief Encrypt payload using AES-256-GCM
 *
 * Encrypts the payload with AES-256-GCM.
 * Generates a random IV and authentication tag.
 *
 * @param[in] payload Payload to encrypt
 * @param[in] payload_len Length of payload
 * @param[in] aes_key AES-256 key
 * @param[out] iv Generated IV (12 bytes)
 * @param[out] ciphertext Output ciphertext
 * @param[out] tag Authentication tag (16 bytes)
 * @return true if successful, false otherwise
 */
static bool address_book_encrypt(uint8_t      *payload,
                                 size_t        payload_len,
                                 const uint8_t aes_key[CX_AES_256_KEY_LEN],
                                 uint8_t       iv[AES_GCM_IV_SIZE],
                                 uint8_t      *ciphertext,
                                 uint8_t       tag[AES_GCM_TAG_SIZE])
{
    cx_aes_gcm_context_t gcm_ctx = {0};
    cx_err_t             error   = CX_INTERNAL_ERROR;
    bool                 success = false;

    // Generate random IV
    if (cx_get_random_bytes(iv, AES_GCM_IV_SIZE) != CX_OK) {
        return false;
    }

    // Initialize GCM context
    cx_aes_gcm_init(&gcm_ctx);

    // Set the AES key
    CX_CHECK(cx_aes_gcm_set_key(&gcm_ctx, aes_key, CX_AES_256_KEY_LEN));

    // Encrypt and generate tag
    CX_CHECK(cx_aes_gcm_encrypt_and_tag(&gcm_ctx,
                                        payload,
                                        payload_len,
                                        iv,
                                        AES_GCM_IV_SIZE,
                                        NULL,  // No additional authenticated data
                                        0,
                                        ciphertext,
                                        tag,
                                        AES_GCM_TAG_SIZE));
    success = true;

end:
    return success;
}

/**
 * @brief Append encrypted data to response buffer
 *
 * Appends the encrypted data in the format:
 * [len_ciphertext(1)][ciphertext][len_iv(1)][iv(12)][len_tag(1)][tag(16)]
 *
 * @param[in, out] tx Current buffer offset, will be updated
 * @param[in] ciphertext Encrypted data
 * @param[in] ciphertext_len Length of ciphertext
 * @param[in] iv Initialization vector (12 bytes)
 * @param[in] tag Authentication tag (16 bytes)
 */
static void address_book_append_encrypted_data(size_t        *tx,
                                               const uint8_t *ciphertext,
                                               size_t         ciphertext_len,
                                               const uint8_t  iv[AES_GCM_IV_SIZE],
                                               const uint8_t  tag[AES_GCM_TAG_SIZE])
{
    // Length of ciphertext (1 byte)
    G_io_tx_buffer[(*tx)++] = (uint8_t) ciphertext_len;
    // Ciphertext
    memmove(&G_io_tx_buffer[*tx], ciphertext, ciphertext_len);
    *tx += ciphertext_len;
    // Length of IV (1 byte) + IV
    G_io_tx_buffer[(*tx)++] = AES_GCM_IV_SIZE;
    memmove(&G_io_tx_buffer[*tx], iv, AES_GCM_IV_SIZE);
    *tx += AES_GCM_IV_SIZE;
    // Length of tag (1 byte) + tag
    G_io_tx_buffer[(*tx)++] = AES_GCM_TAG_SIZE;
    memmove(&G_io_tx_buffer[*tx], tag, AES_GCM_TAG_SIZE);
    *tx += AES_GCM_TAG_SIZE;
}

/** @brief Parse encrypted block: len(1)|ciphertext|len(1)|iv(12)|len(1)|tag(16)
 *
 * @param[in] block Input buffer containing the encrypted block
 * @param[out] ciphertext Pointer to ciphertext within block
 * @param[out] ciphertext_len Length of ciphertext
 * @param[out] iv Pointer to IV within block
 * @param[out] tag Pointer to authentication tag within block
 * @return true if parsing was successful, false otherwise
 */
static bool parse_encrypted_block(const buffer_t *block,
                                  const uint8_t **ciphertext,
                                  size_t         *ciphertext_len,
                                  const uint8_t **iv,
                                  const uint8_t **tag)
{
    const uint8_t *data   = block->ptr;
    uint8_t        len    = 0;
    size_t         offset = 0;

    if (block->size < MIN_ENCRYPTED_BLOCK_SIZE) {
        PRINTF("Block too small: %u bytes (min %u)\n", block->size, MIN_ENCRYPTED_BLOCK_SIZE);
        return false;
    }

    *ciphertext_len = data[offset++];

    if (offset + *ciphertext_len > block->size) {
        return false;
    }
    *ciphertext = &data[offset];
    offset += *ciphertext_len;

    if (offset + 1 > block->size) {
        return false;
    }
    len = data[offset++];
    if (len != AES_GCM_IV_SIZE || offset + len > block->size) {
        return false;
    }
    *iv = &data[offset];
    offset += len;

    if (offset + 1 > block->size) {
        return false;
    }
    len = data[offset++];
    if (len != AES_GCM_TAG_SIZE || offset + len > block->size) {
        return false;
    }
    *tag = &data[offset];

    return true;
}

/**
 * @brief Decrypt and verify with AES-256-GCM
 * Decrypts the ciphertext and verifies the authentication tag. Outputs plaintext if successful.
 *
 * @param[in] ciphertext Data to decrypt
 * @param[in] ciphertext_len Length of ciphertext
 * @param[in] iv Initialization vector
 * @param[in] tag Authentication tag
 * @param[in] aes_key AES-256 key for decryption
 * @param[out] plaintext Output buffer for decrypted data
 * @return true if decryption and verification were successful, false otherwise
 */
static bool decrypt_and_verify(const uint8_t *ciphertext,
                               size_t         ciphertext_len,
                               const uint8_t *iv,
                               const uint8_t *tag,
                               const uint8_t  aes_key[CX_AES_256_KEY_LEN],
                               uint8_t       *plaintext)
{
    cx_aes_gcm_context_t gcm_ctx = {0};
    cx_err_t             error   = CX_INTERNAL_ERROR;
    bool                 success = false;

    cx_aes_gcm_init(&gcm_ctx);
    CX_CHECK(cx_aes_gcm_set_key(&gcm_ctx, aes_key, CX_AES_256_KEY_LEN));
    CX_CHECK(cx_aes_gcm_decrypt_and_auth(&gcm_ctx,
                                         (uint8_t *) ciphertext,
                                         ciphertext_len,
                                         iv,
                                         AES_GCM_IV_SIZE,
                                         NULL,
                                         0,
                                         plaintext,
                                         tag,
                                         AES_GCM_TAG_SIZE));
    success = true;

end:
    return success;
}

/* Functions -----------------------------------------------------------------*/

/**
 * @brief Build name payload with length prefix
 *
 * Format: len(1) | name
 *
 * @param[in] name Input name string
 * @param[out] buffer Output buffer
 * @param[in] buffer_size Size of output buffer
 * @param[out] payload_len Length of the constructed payload
 * @return true if successful, false otherwise
 */
bool build_name_payload(const char *name, uint8_t *buffer, size_t buffer_size, size_t *payload_len)
{
    size_t name_len = strlen(name);

    // Check if name fits in buffer (len byte + name + safety margin for null terminator)
    if (1 + name_len >= buffer_size) {
        PRINTF("Error: Name too long for buffer\n");
        return false;
    }

    // Build payload: len(1) | name
    buffer[0] = (uint8_t) name_len;
    strlcpy((char *) &buffer[1], name, buffer_size - 1);
    *payload_len = 1 + name_len;

    return true;
}

/**
 * @brief Generic function to encrypt and send address book response
 *
 * This function takes pre-built payloads (name and remaining data) and BIP32 paths,
 * performs encryption and sends the encrypted response to the host.
 *
 * Response format: type(1) | name_encrypted_block | remaining_encrypted_block
 *
 * Uses static buffers sized for maximum possible values:
 * - Name: MAX_NAME_LENGTH (33 bytes)
 * - Remaining: MAX_REMAINING_PAYLOAD (108 bytes)
 *
 * @param[in] type Message type (TYPE_EXTERNAL_ADDRESS or TYPE_LEDGER_ACCOUNT)
 * @param[in] bip32_path BIP32 path for encryption
 * @param[in] name_payload Name payload (raw string)
 * @param[in] name_payload_len Length of name payload
 * @param[in] remaining_payload Remaining payload (specific data WITHOUT type)
 * @param[in] remaining_payload_len Length of remaining payload
 * @return true if successful, false otherwise
 */
bool address_book_encrypt_and_send(uint8_t             type,
                                   const path_bip32_t *bip32_path,
                                   uint8_t            *name_payload,
                                   size_t              name_payload_len,
                                   uint8_t            *remaining_payload,
                                   size_t              remaining_payload_len)
{
    cx_ecfp_256_private_key_t private_key                 = {0};
    uint8_t                   aes_key[CX_AES_256_KEY_LEN] = {0};

    uint8_t name_iv[AES_GCM_IV_SIZE]   = {0};
    uint8_t name_tag[AES_GCM_TAG_SIZE] = {0};

    uint8_t remaining_iv[AES_GCM_IV_SIZE]   = {0};
    uint8_t remaining_tag[AES_GCM_TAG_SIZE] = {0};

    cx_err_t error   = CX_INTERNAL_ERROR;
    size_t   tx      = 0;
    bool     success = false;

    // MAX(EXTERNAL_NAME_LENGTH, ACCOUNT_NAME_LENGTH) = 33 bytes
    uint8_t name_ciphertext[MAX_NAME_LENGTH] = {0};

    // MAX_REMAINING_PAYLOAD = 108 bytes (max of external and ledger)
    uint8_t remaining_ciphertext[MAX_REMAINING_PAYLOAD] = {0};

    // Step 1: Derive encryption key (BIP32 secp256r1)
    PRINTF("Deriving encryption key...\n");
    CX_CHECK(bip32_derive_init_privkey_256(
        CX_CURVE_SECP256R1, bip32_path->path, bip32_path->length, &private_key, NULL));

    // Step 2: Derive AES-256 key from private key
    PRINTF("Deriving AES key...\n");
    if (!address_book_derive_aes_key(&private_key, aes_key)) {
        PRINTF("Error: Failed to derive AES key\n");
        goto end;
    }

    // Step 3: Encrypt name
    PRINTF("Encrypting name...\n");
    if (!address_book_encrypt(
            name_payload, name_payload_len, aes_key, name_iv, name_ciphertext, name_tag)) {
        PRINTF("Error: Failed to encrypt name\n");
        goto end;
    }

    // Step 4: Build response buffer
    PRINTF("Building response...\n");
    tx                   = 0;
    G_io_tx_buffer[tx++] = type;

    // Name encrypted block
    address_book_append_encrypted_data(&tx, name_ciphertext, name_payload_len, name_iv, name_tag);

    // Remaining block: only present when there is data to encrypt
    if (remaining_payload != NULL && remaining_payload_len > 0) {
        PRINTF("Encrypting remaining payload...\n");
        if (!address_book_encrypt(remaining_payload,
                                  remaining_payload_len,
                                  aes_key,
                                  remaining_iv,
                                  remaining_ciphertext,
                                  remaining_tag)) {
            PRINTF("Error: Failed to encrypt remaining payload\n");
            goto end;
        }
        address_book_append_encrypted_data(
            &tx, remaining_ciphertext, remaining_payload_len, remaining_iv, remaining_tag);
    }

    // Send the response
    io_send_response_pointer(G_io_tx_buffer, tx, SWO_SUCCESS);
    success = true;

end:
    // Clear sensitive data
    explicit_bzero(&private_key, sizeof(private_key));
    explicit_bzero(aes_key, sizeof(aes_key));
    explicit_bzero(name_ciphertext, sizeof(name_ciphertext));
    explicit_bzero(remaining_ciphertext, sizeof(remaining_ciphertext));

    return success;
}

/**
 * @brief Decrypt an encrypted block from the host
 *
 * @param[in] buffer Encrypted contact name block
 * @param[in] bip32_path BIP32 path for encryption key
 * @param[out] output_name Buffer to store extracted name (EXTERNAL_NAME_LENGTH)
 * @return whether decryption and extraction was successful
 */
bool address_book_decrypt(const buffer_t *buffer, const path_bip32_t *bip32_path, char *output_name)
{
    uint8_t plaintext[MAX_PLAINTEXT_LENGTH] = {0};

    cx_ecfp_256_private_key_t private_key                 = {0};
    uint8_t                   aes_key[CX_AES_256_KEY_LEN] = {0};

    const uint8_t *ct = NULL, *iv = NULL, *tag = NULL;

    size_t   len      = 0;
    uint8_t  name_len = 0;
    cx_err_t error    = CX_INTERNAL_ERROR;
    bool     success  = false;

    // Parse encrypted block
    if (!parse_encrypted_block(buffer, &ct, &len, &iv, &tag)) {
        PRINTF("Failed to parse encrypted block\n");
        goto end;
    }

    // Extract name using length prefix (format: len(1) || name)
    if (len < 1) {
        PRINTF("Error: Decrypted payload too short\n");
        goto end;
    }
    if (len >= sizeof(plaintext)) {
        PRINTF("Error: Decrypted payload too long\n");
        goto end;
    }

    // Derive encryption key
    CX_CHECK(bip32_derive_init_privkey_256(
        CX_CURVE_SECP256R1, bip32_path->path, bip32_path->length, &private_key, NULL));

    if (!address_book_derive_aes_key(&private_key, aes_key)) {
        goto end;
    }

    // Decrypt
    if (!decrypt_and_verify(ct, len, iv, tag, aes_key, plaintext)) {
        PRINTF("Decryption failed\n");
        goto end;
    }

    name_len = plaintext[0];
    if (name_len == 0 || name_len >= EXTERNAL_NAME_LENGTH || (1 + name_len) > len) {
        PRINTF("Error: Invalid name length: %u (total decrypted: %u)\n", name_len, len);
        goto end;
    }

    // Copy extracted name to output buffer
    memmove(output_name, &plaintext[1], name_len);
    output_name[name_len] = '\0';

    success = true;

end:
    explicit_bzero(&private_key, sizeof(private_key));
    explicit_bzero(aes_key, sizeof(aes_key));
    explicit_bzero(plaintext, sizeof(plaintext));

    return success;
}

#endif  // HAVE_ADDRESS_BOOK
