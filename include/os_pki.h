#pragma once
#if defined(HAVE_LEDGER_PKI)
#include "decorators.h"
#include "errors.h"
#include "lcx_ecfp.h"
#include "ox_ec.h"
#include <stdint.h>
#include <stddef.h>

#define CERTIFICATE_FIELD_VAR_LEN              (0xFF)
#define CERTIFICATE_FIELD_UNKNOWN_VALUE        (0xFFFFFFFF)
#define CERTIFICATE_VALIDITY_INDEX             (0x00000001)
#define CERTIFICATE_STRUCTURE_TYPE_CERTIFICATE (0x01)
#define CERTIFICATE_TRUSTED_NAME_MAXLEN        (32)

typedef enum {
    CERTIFICATE_TAG_STRUCTURE_TYPE        = 0x01,
    CERTIFICATE_TAG_VERSION               = 0x02,
    CERTIFICATE_TAG_VALIDITY              = 0x10,
    CERTIFICATE_TAG_VALIDITY_INDEX        = 0x11,
    CERTIFICATE_TAG_CHALLENGE             = 0x12,
    CERTIFICATE_TAG_SIGNER_KEY_ID         = 0x13,
    CERTIFICATE_TAG_SIGN_ALGO_ID          = 0x14,
    CERTIFICATE_TAG_SIGNATURE             = 0x15,
    CERTIFICATE_TAG_TIME_VALIDITY         = 0x16,
    CERTIFICATE_TAG_TRUSTED_NAME          = 0x20,
    CERTIFICATE_TAG_PUBLIC_KEY_ID         = 0x30,
    CERTIFICATE_TAG_PUBLIC_KEY_USAGE      = 0x31,
    CERTIFICATE_TAG_PUBLIC_KEY_CURVE_ID   = 0x32,
    CERTIFICATE_TAG_COMPRESSED_PUBLIC_KEY = 0x33,
    CERTIFICATE_TAG_PK_SIGN_ALGO_ID       = 0x34,
    CERTIFICATE_TAG_TARGET_DEVICE         = 0x35
} os_pki_tag_t;

enum {
    CERTIFICATE_VERSION_02 = 0x02,
    CERTIFICATE_VERSION_UNKNOWN
};

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

enum {
    CERTIFICATE_SIGN_ALGO_ID_ECDSA_SHA256 = 0x00,
    CERTIFICATE_SIGN_ALGO_ID_ECDSA_SHA3,
    CERTIFICATE_SIGN_ALGO_ID_ECDSA_KECCAK,
    CERTIFICATE_SIGN_ALGO_ID_ECDSA_RIPEMD160,
    CERTIFICATE_SIGN_ALGO_ID_EDDSA_SHA512,
    CERTIFICATE_SIGN_ALGO_ID_EDDSA_KECCAK,
    CERTIFICATE_SIGN_ALGO_ID_EDDSA_SHA3,
    CERTIFICATE_SIGN_ALGO_ID_UNKNOWN
};

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

enum {
    CERTIFICATE_TARGET_DEVICE_NANOS = 0x01,
    CERTIFICATE_TARGET_DEVICE_NANOX,
    CERTIFICATE_TARGET_DEVICE_NANOSP,
    CERTIFICATE_TARGET_DEVICE_STAX,
    CERTIFICATE_TARGET_DEVICE_UNKNOWN
};

typedef struct {
    uint32_t value;
    uint8_t  field_len;
} os_pki_certificate_tag_info_t;

// clang-format off
static const os_pki_certificate_tag_info_t C_os_pki_certificate_tag_info[] = {
    [CERTIFICATE_TAG_STRUCTURE_TYPE]        = {CERTIFICATE_STRUCTURE_TYPE_CERTIFICATE, 0x01                     },
    [CERTIFICATE_TAG_VERSION]               = {CERTIFICATE_VERSION_UNKNOWN,            0x01                     },
    [CERTIFICATE_TAG_VALIDITY]              = {CERTIFICATE_FIELD_UNKNOWN_VALUE,        0x04                     },
    [CERTIFICATE_TAG_VALIDITY_INDEX]        = {CERTIFICATE_VALIDITY_INDEX,             0x04                     },
    [CERTIFICATE_TAG_CHALLENGE]             = {CERTIFICATE_FIELD_UNKNOWN_VALUE,        CERTIFICATE_FIELD_VAR_LEN},
    [CERTIFICATE_TAG_SIGNER_KEY_ID]         = {CERTIFICATE_KEY_ID_UNKNOWN,             0x02                     },
    [CERTIFICATE_TAG_SIGN_ALGO_ID]          = {CERTIFICATE_SIGN_ALGO_ID_UNKNOWN,       0x01                     },
    [CERTIFICATE_TAG_TIME_VALIDITY]         = {CERTIFICATE_FIELD_UNKNOWN_VALUE,        0x04                     },
    [CERTIFICATE_TAG_TRUSTED_NAME]          = {CERTIFICATE_FIELD_UNKNOWN_VALUE,        CERTIFICATE_FIELD_VAR_LEN},
    [CERTIFICATE_TAG_PUBLIC_KEY_ID]         = {CERTIFICATE_KEY_ID_UNKNOWN,             0x02                     },
    [CERTIFICATE_TAG_PUBLIC_KEY_USAGE]      = {CERTIFICATE_PUBLIC_KEY_USAGE_UNKNOWN,   0x01                     },
    [CERTIFICATE_TAG_PUBLIC_KEY_CURVE_ID]   = {CX_CURVE_TWISTED_EDWARDS_END,           0x01                     },
    [CERTIFICATE_TAG_COMPRESSED_PUBLIC_KEY] = {CERTIFICATE_KEY_ID_UNKNOWN,             CERTIFICATE_FIELD_VAR_LEN},
    [CERTIFICATE_TAG_PK_SIGN_ALGO_ID]       = {CERTIFICATE_SIGN_ALGO_ID_UNKNOWN,       0x01                     },
    [CERTIFICATE_TAG_TARGET_DEVICE]         = {CERTIFICATE_TARGET_DEVICE_UNKNOWN,      0x01                     },
    [CERTIFICATE_TAG_SIGNATURE]             = {CERTIFICATE_FIELD_UNKNOWN_VALUE,        CERTIFICATE_FIELD_VAR_LEN},
};
// clang-format on

SYSCALL bolos_err_t os_pki_load_certificate(uint8_t               expected_key_usage,
                                            uint8_t *certificate  PLENGTH(certificate_len),
                                            size_t                certificate_len,
                                            uint8_t              *trusted_name,
                                            size_t               *trusted_name_len,
                                            cx_ecfp_public_key_t *public_key);

SYSCALL bool os_pki_verify(uint8_t *descriptor_hash PLENGTH(descriptor_hash_len),
                           size_t                   descriptor_hash_len,
                           uint8_t *signature       PLENGTH(signature_len),
                           size_t                   signature_len);
#endif  // HAVE_LEDGER_PKI
