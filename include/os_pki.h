#pragma once
#if defined(HAVE_LEDGER_PKI)
#include "decorators.h"
#include "errors.h"
#include "lcx_ecfp.h"
#include <stdint.h>
#include <stddef.h>

#define CERTIFICATE_FIELD_VAR_LEN (0xFF)
#define CERTIFICATE_INIT_KEY      (0xFF)

typedef enum {
  CERTIFICATE_TAG_VERSION               = 0x01,
  CERTIFICATE_TAG_VALIDITY              = 0x10,
  CERTIFICATE_TAG_SIGNER_KEY_ID         = 0x13,
  CERTIFICATE_TAG_SIGN_ALGO_ID          = 0x14,
  CERTIFICATE_TAG_CERTIFICATE_NAME      = 0x20,
  CERTIFICATE_TAG_PUBLIC_KEY_ID         = 0x30,
  CERTIFICATE_TAG_PUBLIC_KEY_USAGE      = 0x31,
  CERTIFICATE_TAG_PUBLIC_KEY_CURVE_ID   = 0x32,
  CERTIFICATE_TAG_COMPRESSED_PUBLIC_KEY = 0x33,
  CERTIFICATE_TAG_SIGNATURE             = 0x15
} os_service_tag_t;

typedef struct {
  os_service_tag_t value;
  uint8_t field_len;
} os_service_certificate_tag_info_t;

static const os_service_certificate_tag_info_t C_os_service_certificate_tag_info[] = {
  {CERTIFICATE_TAG_VERSION,               0x01                     },
  {CERTIFICATE_TAG_VALIDITY,              CERTIFICATE_FIELD_VAR_LEN},
  {CERTIFICATE_TAG_SIGNER_KEY_ID,         0x01                     },
  {CERTIFICATE_TAG_SIGN_ALGO_ID,          0x01                     },
  {CERTIFICATE_TAG_CERTIFICATE_NAME,      CERTIFICATE_FIELD_VAR_LEN},
  {CERTIFICATE_TAG_PUBLIC_KEY_ID,         0x01                     },
  {CERTIFICATE_TAG_PUBLIC_KEY_USAGE,      0x01                     },
  {CERTIFICATE_TAG_PUBLIC_KEY_CURVE_ID,   0x01                     },
  {CERTIFICATE_TAG_COMPRESSED_PUBLIC_KEY, 0x21                     },
  {CERTIFICATE_TAG_SIGNATURE,             CERTIFICATE_FIELD_VAR_LEN}
};

#define CERTIFICATE_TAG_INFO_LENGTH (sizeof(C_os_service_certificate_tag_info) / sizeof(C_os_service_certificate_tag_info[0]))

enum {
  CERTIFICATE_SIGN_ALGO_ECDSA_SHA256 = 0,
  CERTIFICATE_SIGN_ALGO_UNKNOWN
};

typedef struct os_pki_public_key_s {
  cx_curve_t curve;
  cx_ecfp_public_key_t public_key;
} os_pki_public_key_t ;

SYSCALL bolos_err_t os_pki_load_certificate(uint8_t expected_key_usage, uint8_t *certificate PLENGTH(certificate_len), size_t certificate_len, cx_ecfp_public_key_t *public_key);
#endif // HAVE_LEDGER_PKI
