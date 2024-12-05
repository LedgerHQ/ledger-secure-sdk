#pragma once

#include "bolos_target.h"
#include "decorators.h"
#include "os_types.h"
#include "lcx_sha256.h"

typedef enum {
    ENDORSEMENT_SLOT_1 = 1,
    ENDORSEMENT_SLOT_2
} ENDORSEMENT_slot_t;

#define ENDORSEMENT_MAX_ASN1_LENGTH (1 + 1 + 2 * (1 + 1 + 33))

// Endorsement fields length
#define ENDORSEMENT_HASH_LENGTH          CX_SHA256_SIZE
#define ENDORSEMENT_METADATA_LENGTH      8
#define ENDORSEMENT_PUBLIC_KEY_LENGTH    65
#define ENDORSEMENT_APP_SECRET_LENGTH    64
#define ENDORSEMENT_SIGNATURE_MAX_LENGTH ENDORSEMENT_MAX_ASN1_LENGTH  // 72

/* ----------------------------------------------------------------------- */
/* -                         ENDORSEMENT FEATURE                         - */
/* ----------------------------------------------------------------------- */

// NEW

bolos_err_t ENDORSEMENT_get_code_hash(uint8_t *out_hash);

bolos_err_t ENDORSEMENT_get_public_key(ENDORSEMENT_slot_t slot,
                                       uint8_t           *out_public_key,
                                       uint8_t           *out_public_key_length);

bolos_err_t ENDORSEMENT_get_public_key_certificate(ENDORSEMENT_slot_t endorsement_slot,
                                                   uint8_t           *out_buffer,
                                                   uint8_t           *out_length);

bolos_err_t ENDORSEMENT_key1_get_app_secret(uint8_t *out_secret);

bolos_err_t ENDORSEMENT_key1_sign_data(uint8_t  *data,
                                       uint32_t  data_length,
                                       uint8_t  *out_signature,
                                       uint32_t *out_signature_length);

uint32_t ENDORSEMENT_key1_sign_without_code_hash(uint8_t  *data,
                                                 uint32_t  data_length,
                                                 uint8_t  *out_signature,
                                                 uint32_t *out_signature_length);

bolos_err_t ENDORSEMENT_key2_derive_sign_data(uint8_t  *data,
                                              uint32_t  data_length,
                                              uint8_t  *out_signature,
                                              uint32_t *out_signature_length);

// OLD

SYSCALL unsigned int os_endorsement_get_code_hash(unsigned char *buffer PLENGTH(32));
SYSCALL unsigned int os_endorsement_get_public_key(unsigned char         index,
                                                   unsigned char *buffer PLENGTH(65),
                                                   unsigned char *length PLENGTH(1));
SYSCALL unsigned int os_endorsement_get_public_key_certificate(
    unsigned char         index,
    unsigned char *buffer PLENGTH(ENDORSEMENT_MAX_ASN1_LENGTH),
    unsigned char *length PLENGTH(1));
SYSCALL unsigned int os_endorsement_key1_get_app_secret(unsigned char *buffer PLENGTH(64));
SYSCALL unsigned int os_endorsement_key1_sign_data(unsigned char *src PLENGTH(srcLength),
                                                   unsigned int       srcLength,
                                                   unsigned char *signature
                                                       PLENGTH(ENDORSEMENT_MAX_ASN1_LENGTH));
SYSCALL unsigned int os_endorsement_key1_sign_without_code_hash(
    unsigned char *src       PLENGTH(srcLength),
    unsigned int             srcLength,
    unsigned char *signature PLENGTH(ENDORSEMENT_MAX_ASN1_LENGTH));
SYSCALL unsigned int os_endorsement_key2_derive_sign_data(unsigned char *src PLENGTH(srcLength),
                                                          unsigned int       srcLength,
                                                          unsigned char *signature
                                                              PLENGTH(ENDORSEMENT_MAX_ASN1_LENGTH));

SYSCALL unsigned int os_endorsement_get_metadata(unsigned char         index,
                                                 unsigned char *buffer PLENGTH(8));
