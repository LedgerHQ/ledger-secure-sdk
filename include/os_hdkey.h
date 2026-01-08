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

/*********************
 *      DEFINES
 *********************/


/**********************
 *      TYPEDEFS
 **********************/


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
