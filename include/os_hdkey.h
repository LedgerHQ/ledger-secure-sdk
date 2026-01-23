/**
 * @file os_hdkey.h
 * @brief Header file containing prototypes for hierarchical deterministic key derivation
 */

#pragma once

/*********************
 *      INCLUDES
 *********************/
#include <stdint.h>
#include <stddef.h>
#include "decorators.h"
#include "os_types.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

typedef enum HDKEY_derive_auth_e {
    HDKEY_DERIVE_AUTH_256K1 = 0x01u,
    HDKEY_DERIVE_AUTH_256R1 = 0x02u,
    HDKEY_DERIVE_AUTH_ED25519 = 0x04u,
    HDKEY_DERIVE_AUTH_SLIP21 = 0x08u,
    HDKEY_DERIVE_AUTH_BLS12381G1 = 0x10u,
    HDKEY_DERIVE_AUTH_BLS12377G1 = 0x20u
} HDKEY_derive_auth_t;

typedef enum HDKEY_derive_mode_e {
    HDKEY_DERIVE_MODE_NORMAL = 0x00u,
    HDKEY_DERIVE_MODE_ED25519_SLIP10  = 0x01u,
    HDKEY_DERIVE_MODE_SLIP21  = 0x02u,
    HDKEY_DERIVE_MODE_RESERVED = 0x04u,
    HDKEY_DERIVE_MODE_BLS12377_ALEO = 0x08u
} HDKEY_derive_mode_t;


/**********************
 * GLOBAL PROTOTYPES
 **********************/

SYSCALL bolos_err_t sys_hdkey_derive(HDKEY_derive_mode_t derivation_mode,
                                     cx_curve_t          curve,
                                     const uint32_t     *path,
                                     size_t              path_len,
                                     uint8_t            *private_key,
                                     size_t              private_key_len,
                                     uint8_t            *chain_code,
                                     size_t              chain_code_len,
                                     uint8_t            *seed,
                                     size_t              seed_len);
