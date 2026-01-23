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
 * @param[out] hash Input byte buffer where to write application hash.
 * @param[in] length Hash buffer length. Must be greater or equal to ENDORSEMENT_HASH_LENGTH
 *
 * @return bolos_err_t
 * @retval 0x5503 Pin is not validated
 * @retval 0x5116 Internal error
 * @retval 0x522F Internal error
 * @retval 0x0000 Success
 */
SYSCALL bolos_err_t sys_endorsement_get_code_hash(uint8_t *hash, size_t length);

/**
 * @brief Retrieve endorsement public key from given slot
 *
 * @param[in] slot Index of the slot from which the public key is read.
 * @param[out] public_key Output byte buffer where to write public key corresponding to the slot.
 * @param[in, out] public_key_length Public key length in bytes.
 * On entry, it must contain the size of the public_key buffer. Must be greater or equal to
 * ENDORSEMENT_PUBLIC_KEY_LENGTH. On return, it contains the actual size in bytes written to
 * public_key buffer.
 *
 * @retval 0x5218: Endorsement not set or corrupted
 * @retval 0x4209: \p slot is invalid
 * @retval 0x4103: No private key is loaded in slot
 * @retval 0x5415: Error during public key structure init
 * @retval 0x5416: Error during public key computation
 * @retval 0x0000: Success
 */
SYSCALL bolos_err_t sys_endorsement_get_public_key(ENDORSEMENT_slot_t slot,
                                                   uint8_t           *public_key,
                                                   size_t            *public_key_length);

/**
 * @brief Get public key signature from given slot
 *
 * @param[in] slot Index of the slot from which the certificate public key is to be read.
 * @param[out] sig Output byte buffer where to write the endorsement signature.
 * @param[in, out] sig_length Signature length in bytes.
 * On entry, it must contain the size of the signature input buffer. Must be greater or equal to
 * ENDORSEMENT_SIGNATURE_MAX_LENGTH. On return, in case of success, it contains the actual number of
 * bytes written to signature.
 *
 * @return bolos_err_t
 * @retval 0x5219: Endorsement not set or corrupted
 * @retval 0x420A: \p slot is invalid
 * @retval 0x4104: \p slot is not set
 * @retval 0x0000: Success
 */
SYSCALL bolos_err_t sys_endorsement_get_public_key_signature(ENDORSEMENT_slot_t slot,
                                                             uint8_t           *sig,
                                                             size_t            *sig_length);

/**
 * @brief Compute secret from current app code hash and slot1 secret field
 *
 * @param[out] secret Input byte buffer where to write secret.
 * @param[in] length Secret buffer length in bytes. Must be greater or equal to
 * ENDORSEMENT_APP_SECRET_LENGTH.
 *
 * @return bolos_err_t
 * @retval 0x521A Endorsement not set or corrupted
 * @retval 0x4105 No private key set in slot 1
 * @retval 0x534A Error during hmac computation
 * @retval 0x0000 Success
 */
SYSCALL bolos_err_t sys_endorsement_key1_get_app_secret(uint8_t *secret, size_t length);

/**
 * @brief Perform hash and sign on input data and calling app code hash.
 *
 * Hash input data, update the hash with current calling app code hash, then sign it with keys from
 * endorsement slot 1.
 *
 * @param[in] data Input byte buffer holding data to be hashed
 * @param[in] data_length Data length in bytes
 * @param[out] sig Output byte buffer where resulting signature is written.
 * @param[in, out] sig_length Signature length in bytes.
 * On entry, it must contain the size of the signature input buffer. Must be greater or equal to
 * ENDORSEMENT_SIGNATURE_MAX_LENGTH. On return, in case of success, it contains the actual number of
 * bytes written to signature.
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
SYSCALL bolos_err_t sys_endorsement_key1_sign_data(const uint8_t *data,
                                                   size_t         data_length,
                                                   uint8_t       *sig,
                                                   size_t        *sig_length);

/**
 * @brief Hash input data, then sign the hash with slot1 key. App code hash is not
 * included when computing hash.
 *
 * @param[in] data Input byte buffer holding data to be hashed
 * @param[in] data_length Data length in bytes
 * @param[out] sig Output byte buffer where resulting signature is written.
 * @param[in, out] sig_length Signature length in bytes.
 * On entry, it must contain the size of the signature input buffer. Must be greater or equal to
 * ENDORSEMENT_SIGNATURE_MAX_LENGTH. On return, in case of success, it contains the actual number of
 * bytes written to signature.
 *
 * @return bolos_err_t
 * @retval 0x522E Invalid endorsement structure
 * @retval 0x4117 No private key is loaded in slot 1
 * @retval 0xFFFFFFxx Cryptography-related error
 * @retval 0x0000 Success
 */
SYSCALL bolos_err_t sys_endorsement_key1_sign_without_code_hash(const uint8_t *data,
                                                                size_t         data_length,
                                                                uint8_t       *sig,
                                                                size_t        *sig_length);

/**
 * @brief Perform hashing and signature on input data and slot 2 derived keys.
 *
 * @param[in] data Input byte buffer holading data to be hashed and signed
 * @param[in] data_length Data length in bytes
 * @param[out] sig Output byte buffer where resulting signature is written.
 * @param[in, out] sig_length Signature length in bytes.
 * On entry, it must contain the size of the signature input buffer. Must be greater or equal to
 * ENDORSEMENT_SIGNATURE_MAX_LENGTH. On return, in case of success, it contains the actual number of
 * bytes written to signature.
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
SYSCALL bolos_err_t sys_endorsement_key2_derive_and_sign_data(const uint8_t *data,
                                                              size_t         data_length,
                                                              uint8_t       *sig,
                                                              size_t        *sig_length);

/**
 * @brief Get metadata from given slot
 *
 * @param slot Index of slot from which metadata is read
 * @param[out] metadata Output byte buffer where metadata will be written.
 * @param[in, out] metadata_length Metadata length in bytes.
 * On entry, it must contain the size of the metadata input buffer. Must be greater or equal to
 * ENDORSEMENT_METADATA_LENGTH. On return, in case of success, it contains the actual number of
 * bytes written to metadata.
 *
 * @return
 * @retval 0x5222 Endorsement not set or corrupted
 * @retval 0x4225 Invalid \p slot parameter
 */
SYSCALL bolos_err_t sys_endorsement_get_metadata(ENDORSEMENT_slot_t slot,
                                                 uint8_t           *metadata,
                                                 size_t            *metadata_length);

/**********************
 *      MACROS
 **********************/
