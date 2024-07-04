#pragma once
#if defined(HAVE_LEDGER_PKI)
#include "decorators.h"
#include "errors.h"
#include "lcx_ecfp.h"
#include "ox_ec.h"
#include <stdint.h>
#include <stddef.h>

/** Certificate field with a variable length */
#define CERTIFICATE_FIELD_VAR_LEN              (0xFF)
/** Certificate field with a non predefined value */
#define CERTIFICATE_FIELD_UNKNOWN_VALUE        (0xFFFF)
/** Certificate validity index minimum value */
#define CERTIFICATE_VALIDITY_INDEX             (0x0001)
/** Certificate structure type */
#define CERTIFICATE_STRUCTURE_TYPE_CERTIFICATE (0x01)
/** Maximum certificate trusted name length */
#define CERTIFICATE_TRUSTED_NAME_MAXLEN        (32)
/** Certificate depth maximum value */
#define CERTIFICATE_DEPTH_MAX_VALUE            (0xFF)

/** Certificate tags associated to each certificate field */
typedef enum {
    CERTIFICATE_TAG_STRUCTURE_TYPE        = 0x01,  ///< Structure type
    CERTIFICATE_TAG_VERSION               = 0x02,  ///< Certificate version
    CERTIFICATE_TAG_VALIDITY              = 0x10,  ///< Certificate validity
    CERTIFICATE_TAG_VALIDITY_INDEX        = 0x11,  ///< Certificate validity index
    CERTIFICATE_TAG_CHALLENGE             = 0x12,  ///< Challenge value
    CERTIFICATE_TAG_SIGNER_KEY_ID         = 0x13,  ///< Signer key ID
    CERTIFICATE_TAG_SIGN_ALGO_ID          = 0x14,  ///< Signature algorithm with the signer key
    CERTIFICATE_TAG_SIGNATURE             = 0x15,  ///< Signature
    CERTIFICATE_TAG_TIME_VALIDITY         = 0x16,  ///< Time validity
    CERTIFICATE_TAG_TRUSTED_NAME          = 0x20,  ///< Trusted name
    CERTIFICATE_TAG_PUBLIC_KEY_ID         = 0x30,  ///< Public key ID
    CERTIFICATE_TAG_PUBLIC_KEY_USAGE      = 0x31,  ///< Public key usage
    CERTIFICATE_TAG_PUBLIC_KEY_CURVE_ID   = 0x32,  ///< Curve ID on which the public key is defined
    CERTIFICATE_TAG_COMPRESSED_PUBLIC_KEY = 0x33,  ///< Public key in compressed form
    CERTIFICATE_TAG_PK_SIGN_ALGO_ID       = 0x34,  ///< Signature algorithm with the public key
    CERTIFICATE_TAG_TARGET_DEVICE         = 0x35,  ///< Target device
    CERTIFICATE_TAG_DEPTH                 = 0x36   ///< Certificate depth
} os_pki_tag_t;

/** Certificate version possible values */
enum {
    CERTIFICATE_VERSION_02 = 0x02,  ///< Certificate version 2
    CERTIFICATE_VERSION_UNKNOWN
};

/** Certificate key ID possible values */
enum {
    CERTIFICATE_KEY_ID_TEST = 0x0000,
    CERTIFICATE_KEY_ID_PERSOV2,
    CERTIFICATE_KEY_ID_LEDGER_ROOT_V3,
    CERTIFICATE_KEY_ID_PLUGIN_SELECTOR,
    CERTIFICATE_KEY_ID_NFT_METADATA,
    CERTIFICATE_KEY_ID_PARTNER_METADATA,
    CERTIFICATE_KEY_ID_ERC20_METADATA,
    CERTIFICATE_KEY_ID_DOMAIN_METADATA,
    CERTIFICATE_KEY_ID_UNKNOWN
};

/** Signature algorithm possible values */
enum {
    CERTIFICATE_SIGN_ALGO_ID_ECDSA_SHA256     = 0x01,
    CERTIFICATE_SIGN_ALGO_ID_ECDSA_SHA3_256   = 0x02,
    CERTIFICATE_SIGN_ALGO_ID_ECDSA_KECCAK_256 = 0x03,
    CERTIFICATE_SIGN_ALGO_ID_ECDSA_RIPEMD160  = 0x04,
    CERTIFICATE_SIGN_ALGO_ID_EDDSA_SHA512     = 0x10,
    CERTIFICATE_SIGN_ALGO_ID_UNKNOWN
};

/** Public key usages possible values */
enum {
    CERTIFICATE_PUBLIC_KEY_USAGE_GENUINE_CHECK = 0x01,
    CERTIFICATE_PUBLIC_KEY_USAGE_EXCHANGE_PAYLOAD,
    CERTIFICATE_PUBLIC_KEY_USAGE_NFT_METADATA,
    CERTIFICATE_PUBLIC_KEY_USAGE_TRUSTED_NAME,
    CERTIFICATE_PUBLIC_KEY_USAGE_BACKUP_PROVIDER,
    CERTIFICATE_PUBLIC_KEY_USAGE_RECOVER_ORCHESTRATOR,
    CERTIFICATE_PUBLIC_KEY_USAGE_PLUGIN_METADATA,
    CERTIFICATE_PUBLIC_KEY_USAGE_COIN_META,
    CERTIFICATE_PUBLIC_KEY_USAGE_SEED_ID_AUTH,
    CERTIFICATE_PUBLIC_KEY_USAGE_UNKNOWN,
};

/** Target device possible values */
enum {
    CERTIFICATE_TARGET_DEVICE_NANOS = 0x01,
    CERTIFICATE_TARGET_DEVICE_NANOX,
    CERTIFICATE_TARGET_DEVICE_NANOSP,
    CERTIFICATE_TARGET_DEVICE_STAX,
    CERTIFICATE_TARGET_DEVICE_FLEX,
    CERTIFICATE_TARGET_DEVICE_UNKNOWN
};

/** Structure to store field length and field maximum value */
typedef struct {
    uint16_t value;
    uint8_t  field_len;
} os_pki_certificate_tag_info_t;

/** Indices for #C_os_pki_certificate_tag_info */
enum {
    CERTIFICATE_INFO_INDEX_STRUCTURE_TYPE = 0,
    CERTIFICATE_INFO_INDEX_VERSION,
    CERTIFICATE_INFO_INDEX_VALIDITY,
    CERTIFICATE_INFO_INDEX_VALIDITY_INDEX,
    CERTIFICATE_INFO_INDEX_CHALLENGE,
    CERTIFICATE_INFO_INDEX_SIGNER_KEY_ID,
    CERTIFICATE_INFO_INDEX_SIGN_ALGO_ID,
    CERTIFICATE_INFO_INDEX_TIME_VALIDITY,
    CERTIFICATE_INFO_INDEX_TRUSTED_NAME,
    CERTIFICATE_INFO_INDEX_PUBLIC_KEY_ID,
    CERTIFICATE_INFO_INDEX_PUBLIC_KEY_USAGE,
    CERTIFICATE_INFO_INDEX_PUBLIC_KEY_CURVE_ID,
    CERTIFICATE_INFO_INDEX_COMPRESSED_PUBLIC_KEY,
    CERTIFICATE_INFO_INDEX_PK_SIGN_ALGO_ID,
    CERTIFICATE_INFO_INDEX_TARGET_DEVICE,
    CERTIFICATE_INFO_INDEX_SIGNATURE,
    CERTIFICATE_INFO_INDEX_DEPTH
};

// clang-format off
/** Array of field length and field maximum value corresponding to each tag */
static const os_pki_certificate_tag_info_t C_os_pki_certificate_tag_info[] = {
    [CERTIFICATE_INFO_INDEX_STRUCTURE_TYPE]        = {CERTIFICATE_STRUCTURE_TYPE_CERTIFICATE,   0x01                     },
    [CERTIFICATE_INFO_INDEX_VERSION]               = {CERTIFICATE_VERSION_UNKNOWN,              0x01                     },
    [CERTIFICATE_INFO_INDEX_VALIDITY]              = {CERTIFICATE_FIELD_UNKNOWN_VALUE,          0x04                     },
    [CERTIFICATE_INFO_INDEX_VALIDITY_INDEX]        = {CERTIFICATE_VALIDITY_INDEX,               0x04                     },
    [CERTIFICATE_INFO_INDEX_CHALLENGE]             = {CERTIFICATE_FIELD_UNKNOWN_VALUE,          CERTIFICATE_FIELD_VAR_LEN},
    [CERTIFICATE_INFO_INDEX_SIGNER_KEY_ID]         = {CERTIFICATE_KEY_ID_UNKNOWN,               0x02                     },
    [CERTIFICATE_INFO_INDEX_SIGN_ALGO_ID]          = {CERTIFICATE_SIGN_ALGO_ID_ECDSA_RIPEMD160, 0x01                     },
    [CERTIFICATE_INFO_INDEX_TIME_VALIDITY]         = {CERTIFICATE_FIELD_UNKNOWN_VALUE,          0x04                     },
    [CERTIFICATE_INFO_INDEX_TRUSTED_NAME]          = {CERTIFICATE_FIELD_UNKNOWN_VALUE,          CERTIFICATE_FIELD_VAR_LEN},
    [CERTIFICATE_INFO_INDEX_PUBLIC_KEY_ID]         = {CERTIFICATE_KEY_ID_UNKNOWN,               0x02                     },
    [CERTIFICATE_INFO_INDEX_PUBLIC_KEY_USAGE]      = {CERTIFICATE_PUBLIC_KEY_USAGE_UNKNOWN,     0x01                     },
    [CERTIFICATE_INFO_INDEX_PUBLIC_KEY_CURVE_ID]   = {CX_CURVE_TWISTED_EDWARDS_END,             0x01                     },
    [CERTIFICATE_INFO_INDEX_COMPRESSED_PUBLIC_KEY] = {CERTIFICATE_FIELD_UNKNOWN_VALUE,          CERTIFICATE_FIELD_VAR_LEN},
    [CERTIFICATE_INFO_INDEX_PK_SIGN_ALGO_ID]       = {CERTIFICATE_SIGN_ALGO_ID_UNKNOWN,         0x01                     },
    [CERTIFICATE_INFO_INDEX_TARGET_DEVICE]         = {CERTIFICATE_TARGET_DEVICE_UNKNOWN,        0x01                     },
    [CERTIFICATE_INFO_INDEX_SIGNATURE]             = {CERTIFICATE_FIELD_UNKNOWN_VALUE,          CERTIFICATE_FIELD_VAR_LEN},
    [CERTIFICATE_INFO_INDEX_DEPTH]                 = {CERTIFICATE_DEPTH_MAX_VALUE,              0x01                     },
};

static const cx_md_t C_os_sign_algo_hash_info[] = {
    [CERTIFICATE_SIGN_ALGO_ID_ECDSA_SHA256]     = CX_SHA256,
    [CERTIFICATE_SIGN_ALGO_ID_ECDSA_SHA3_256]   = CX_SHA3_256,
    [CERTIFICATE_SIGN_ALGO_ID_ECDSA_KECCAK_256] = CX_KECCAK,
    [CERTIFICATE_SIGN_ALGO_ID_ECDSA_RIPEMD160]  = CX_RIPEMD160
};
// clang-format on

/**
 * @brief Load a certificate and initialize the public key on success.
 *
 * @param[in]  expected_key_usage Key verification role.
 * @param[in]  certificate_len    Certificate length.
 * @param[in]  certificate_len    Certificate
 * @param[out] trusted_name       Trusted name from the certificate
 * @param[out] trusted_name_len   Trusted name length
 * @param[out] public_key         Initialized public key from the certificate
 *
 * @return Error code
 * @retval 0x0000 Success
 * @retval 0x422F Incorrect structure type
 * @retval 0x4230 Incorrect certificate version
 * @retval 0x4231 Incorrect certificate validity
 * @retval 0x4232 Incorrect certificate validity index
 * @retval 0x4233 Unknown signer key ID
 * @retval 0x4234 Unknown signature algorithm
 * @retval 0x4235 Unknown public key ID
 * @retval 0x4236 Unknown public key usage
 * @retval 0x4237 Incorrect elliptic curve ID
 * @retval 0x4238 Incorrect signature algorithm associated to the public key
 * @retval 0x4239 Unknown target device
 * @retval 0x422D Unknown certificate tag
 * @retval 0x3301 Failed to hash data
 * @retval 0x422E expected_key_usage doesn't match certificate key usage
 * @retval 0x5720 Failed to verify signature
 * @retval 0x4118 trusted_name buffer is too small to contain the trusted name
 * @retval 0xFFFFFFxx Cryptography-related error
 */
SYSCALL bolos_err_t os_pki_load_certificate(uint8_t                   expected_key_usage,
                                            uint8_t *certificate      PLENGTH(certificate_len),
                                            size_t                    certificate_len,
                                            uint8_t                  *trusted_name,
                                            size_t                   *trusted_name_len,
                                            cx_ecfp_384_public_key_t *public_key);

/**
 * @brief Verify a descriptor signature with internal public key.
 *
 * @details The 'load certificate' command must be sent before this function
 *          to initialize the internal public key.
 *          The caller is responsible for computing the descriptor hash prior
 *          to the verification.
 *
 * @param[in] descriptor_hash     Hash of a descriptor
 * @param[in] descriptor_hash_len Length of the descriptor hash
 * @param[in] signature           Signature over the descriptor
 * @param[in] signature_len       Signature length
 * @return bool
 * @retval true Success
 * @retval false Failed to verify
 */
SYSCALL bool os_pki_verify(uint8_t *descriptor_hash PLENGTH(descriptor_hash_len),
                           size_t                   descriptor_hash_len,
                           uint8_t *signature       PLENGTH(signature_len),
                           size_t                   signature_len);

/**
 * @brief Get information from the last validated certificate.
 *
 * @param[out] key_usage        Certificate role (expected key usage)
 * @param[out] trusted_name     Buffer for the trusted name.
 *                              The size of the buffer must be less than
 *                              #CERTIFICATE_TRUSTED_NAME_MAXLEN
 * @param[out] trusted_name_len Trusted name length
 * @param[out] public_key       Certificate public key
 * @return Error code
 * @retval 0x0000 Success
 * @retval 0x4119 trusted_name buffer is too small to contain the trusted name
 * @retval 0x3202 Failed to initialize public key
 */
SYSCALL bolos_err_t os_pki_get_info(uint8_t                  *key_usage,
                                    uint8_t                  *trusted_name,
                                    size_t                   *trusted_name_len,
                                    cx_ecfp_384_public_key_t *public_key);
#endif  // HAVE_LEDGER_PKI
