/**
 * @file os_endorsement.h
 * @brief Header file containing all prototypes related to endorsement feature
 */

#pragma once

/*********************
 *      INCLUDES
 *********************/
#include <stdint.h>
#include <stddef.h>

#include "decorators.h"
#include "os_types.h"
#include "lcx_sha256.h"

/*********************
 *      DEFINES
 *********************/

#define ENDORSEMENT_HASH_LENGTH          CX_SHA256_SIZE
#define ENDORSEMENT_METADATA_LENGTH      8U
#define ENDORSEMENT_PUBLIC_KEY_LENGTH    65U
#define ENDORSEMENT_APP_SECRET_LENGTH    64U
#define ENDORSEMENT_MAX_ASN1_LENGTH      72U
#define ENDORSEMENT_SIGNATURE_MAX_LENGTH ENDORSEMENT_MAX_ASN1_LENGTH

/**********************
 *      TYPEDEFS
 **********************/

enum ENDORSEMENT_slot_e {
    ENDORSEMENT_SLOT_INVALID,
    ENDORSEMENT_SLOT_1 = 1U,
    ENDORSEMENT_SLOT_2 = 2U
};

typedef uint8_t ENDORSEMENT_slot_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * @brief Get sha256 hash of currently running application.
 *
 * @param[out] out_hash Pointer where application hash is copied. Writing is performed only if
 * not-NULL. Hash has a fixed size of ENDORSEMENT_HASH_LENGTH.
 *
 * @return bolos_err_t
 * @retval 0x5503 Pin is not validated
 * @retval 0x5116 Internal error
 * @retval 0x522F Internal error
 * @retval 0x0000 Success
 */
SYSCALL bolos_err_t ENDORSEMENT_get_code_hash(uint8_t *out_hash);

/**
 * @brief Compute endorsement public key from given slot
 *
 * @param[in] slot Index of the slot from which the public key is read
 * @param[out] out_public_key Pointer where public key is written. (Size
 * ENDORSEMENT_PUBLIC_KEY_LENGTH)
 * @param[out] out_public_key_length Point where public key length is written.
 *
 * @retval 0x5218: Endorsement not set or corrupted
 * @retval 0x4209: \p slot is invalid
 * @retval 0x4103: No private key is loaded in slot
 * @retval 0x5415: Error during public key structure init
 * @retval 0x5416: Error during public key computation
 * @retval 0x0000: Success
 */
SYSCALL bolos_err_t ENDORSEMENT_get_public_key(ENDORSEMENT_slot_t slot,
                                               uint8_t           *out_public_key,
                                               uint8_t           *out_public_key_length);

/**
 * @brief Get certificate public key from given slot
 *
 * @param[in] slot Index of the slot from which the certificate public key is to be read.
 * @param[out] out_buffer Pointer where certificate content is written. Writing is performed only if
 * not NULL. Size ENDORSEMENT_SIGNATURE_MAX_LENGTH.
 * @param[out] out_length Pointer where certificate length is written. Writing is performed only if
 * not NULL.
 *
 * @return bolos_err_t
 * @retval 0x5219: Endorsement not set or corrupted
 * @retval 0x420A: \p slot is invalid
 * @retval 0x4104: \p slot is not set
 * @retval 0x0000: Success
 */
SYSCALL bolos_err_t ENDORSEMENT_get_public_key_certificate(ENDORSEMENT_slot_t slot,
                                                           uint8_t           *out_buffer,
                                                           uint8_t           *out_length);

/**
 * @brief Compute secret from current app code hash and slot1 secret field
 *
 * @param[out] out_secret Pointer where resulting secret is written. App secret has a fixed length
 * of ENDORSEMENT_APP_SECRET_LENGTH.
 *
 * @return bolos_err_t
 * @retval 0x521A Endorsement not set or corrupted
 * @retval 0x4105 No private key set in slot 1
 * @retval 0x534A Error during hmac computation
 * @retval 0x0000 Success
 */
SYSCALL bolos_err_t ENDORSEMENT_key1_get_app_secret(uint8_t *out_secret);

/**
 * @brief Perform hash and sign on input data and calling app code hash.
 *
 * Hash input data, update the hash with current calling app code hash, then sign it with keys from
 * endorsement slot 1.
 *
 * @param[in] data Data to be hashed
 * @param[in] dataLength Length of \p data
 * @param[out] out_signature Pointer where resulting signature is written (size
 * ENDORSEMENT_HASH_LENGTH)
 * @param[out] out_signature_length If non-NULL, pointer where resulting signature length is
 * written.
 *
 * @return bolos_err_t
 * @retval 0x521B Endorsement not set or corrupted
 * @retval 0x5503 Pin is not validated
 * @retval 0x5116 Internal error
 * @retval 0x522F Internal error
 * @retval 0x4106 No private key is set in slot 1
 * @retval 0xFFFFFFxx Cryptography-related error
 * @retval 0x0000 Success
 */
SYSCALL bolos_err_t ENDORSEMENT_key1_sign_data(uint8_t  *data,
                                               uint32_t  data_length,
                                               uint8_t  *out_signature,
                                               uint32_t *out_signature_length);

/**
 * @brief Hash input data, then sign the hash with slot1 key. App code hash is not
 * included when computing hash.
 *
 * @param[in] data Data to be hashed and signed
 * @param[in] data_length Length of \p data
 * @param[out] out_signature Pointer where resulting signature is written.
 * @param[out] out_signature_length If non-NULL, pointer where resulting signature length is
 * written.
 *
 * @return bolos_err_t
 * @retval 0x522E Invalid endorsement structure
 * @retval 0x4117 No private key is loaded in slot 1
 * @retval 0xFFFFFFxx Cryptography-related error
 * @retval 0x0000 Success
 */
SYSCALL bolos_err_t ENDORSEMENT_key1_sign_without_code_hash(uint8_t  *data,
                                                            uint32_t  data_length,
                                                            uint8_t  *out_signature,
                                                            uint32_t *out_signature_length);

/**
 * @brief Perform hashing and signature on input data and slot 2 derived keys.
 * @note Internal implementation of `ENDORSEMENT_key2_derive_sign_data` syscall
 *
 * @param[in] data Input data to be hashed and signed
 * @param[in] data_length Length of \p data
 * @param[out] out_signature Pointer where resulting signature is written (size
 * ENDORSEMENT_SIGNATURE_MAX_LENGTH).
 * @param[out] out_signature_length If non-NULL, pointer where resulting signature length is
 * written.
 *
 * @return bolos_err_t
 * @retval 0x521C Endorsement not set or corrupted
 * @retval 0x5503 Pin is not validated
 * @retval 0x5116 Internal error
 * @retval 0x522F Internal error
 * @retval 0x4107 No private key is loaded in slot 2
 * @retval 0x5716 Cryptographic operation failed
 * @retval 0x0000 Success
 */
SYSCALL bolos_err_t ENDORSEMENT_key2_derive_and_sign_data(uint8_t  *data,
                                                          uint32_t  data_length,
                                                          uint8_t  *out_signature,
                                                          uint32_t *out_signature_length);

/**
 * @brief Get metadata from given slot
 * @note Internal implementation of `os_endorsement_get_metadata` syscall
 *
 * @param slot Index of slot from which metadata is read
 * @param[in] out_buffer Pointer where metadata are written (size ENDORSEMENT_METADTA_LENGTH)
 * @param[out] out_length Pointer where metadata length is written
 *
 * @return
 * @retval 0x5222 Endorsement not set or corrupted
 * @retval 0x4225 Invalid \p slot parameter
 */
SYSCALL bolos_err_t ENDORSEMENT_get_metadata(ENDORSEMENT_slot_t slot,
                                             uint8_t           *out_metadata,
                                             uint8_t           *out_metadata_length);

/**********************
 *      MACROS
 **********************/
