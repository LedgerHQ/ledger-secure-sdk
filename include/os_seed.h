#pragma once

#include <string.h>

#include "appflags.h"
#include "decorators.h"
#include "lcx_ecfp.h"
#include "os_types.h"

// checked in the ux flow to avoid asking the pin for example
// NBA : could also be checked by applications running in insecure mode - thus unprivilegied
// @return BOLOS_UX_OK when perso is onboarded.
SYSCALL bolos_bool_t os_perso_isonboarded(void);

// derive the seed for the requested BIP32 path
// Deprecated : see "os_derive_bip32_no_throw"
#ifndef HAVE_BOLOS
DEPRECATED
#endif
SYSCALL void os_perso_derive_node_bip32(cx_curve_t curve,
                                        const unsigned int *path
                                                     PLENGTH(4 * (pathLength & 0x0FFFFFFFu)),
                                        unsigned int pathLength,
                                        unsigned char *privateKey PLENGTH(64),
                                        unsigned char *chain      PLENGTH(32));

#define HDW_NORMAL         0
#define HDW_ED25519_SLIP10 1
// symmetric key derivation according to SLIP-0021
// this only supports derivation of the master node (level 1)
// the beginning of the authorized path is to be provided in the authorized derivation tag of the
// registry starting with a \x00 Note: for SLIP21, the path is a string and the pathLength is the
// number of chars including the starting \0 byte. However, firewall checks are processing a number
// of integers, therefore, take care not to locate the buffer too far in memory to pass the firewall
// check.
#define HDW_SLIP21         2

// derive the seed for the requested BIP32 path, with the custom provided seed_key for the sha512
// hmac ("Bitcoin Seed", "Nist256p1 Seed", "ed25519 seed", ...) Deprecated : see
// "os_derive_bip32_with_seed_no_throw"
#ifndef HAVE_BOLOS
DEPRECATED
#endif
SYSCALL void os_perso_derive_node_with_seed_key(unsigned int mode,
                                                cx_curve_t   curve,
                                                const unsigned int *path
                                                    PLENGTH(4 * (pathLength & 0x0FFFFFFFu)),
                                                unsigned int              pathLength,
                                                unsigned char *privateKey PLENGTH(64),
                                                unsigned char *chain      PLENGTH(32),
                                                unsigned char *seed_key   PLENGTH(seed_key_length),
                                                unsigned int              seed_key_length);

#define os_perso_derive_node_bip32_seed_key os_perso_derive_node_with_seed_key

/**
 * @brief   Gets the private key from the device seed using the specified bip32 path and seed key.
 *
 * @param[in]  derivation_mode Derivation mode, one of HDW_NORMAL / HDW_ED25519_SLIP10 / HDW_SLIP21.
 *
 * @param[in]  curve           Curve identifier.
 *
 * @param[in]  path            Bip32 path to use for derivation.
 *
 * @param[in]  path_len        Bip32 path length.
 *
 * @param[out] raw_privkey     Buffer where to store the private key.
 *
 * @param[out] chain_code      Buffer where to store the chain code. Can be NULL.
 *
 * @param[in]  seed            Seed key to use for derivation.
 *
 * @param[in]  seed_len        Seed key length.
 *
 * @return                     Error code:
 *                             - CX_OK on success
 *                             - CX_INTERNAL_ERROR
 */
WARN_UNUSED_RESULT static inline cx_err_t os_derive_bip32_with_seed_no_throw(
    unsigned int        derivation_mode,
    cx_curve_t          curve,
    const unsigned int *path,
    unsigned int        path_len,
    unsigned char       raw_privkey[static 64],
    unsigned char      *chain_code,
    unsigned char      *seed,
    unsigned int        seed_len)
{
    cx_err_t error = CX_OK;

    BEGIN_TRY
    {
        TRY
        {
// ignore the deprecated warning, pragma to remove when the "no throw" OS function will be available
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
            // Derive the seed with path
            os_perso_derive_node_bip32_seed_key(
                derivation_mode, curve, path, path_len, raw_privkey, chain_code, seed, seed_len);
#pragma GCC diagnostic pop
        }
        CATCH_OTHER(e)
        {
            error = e;

            // Make sure the caller doesn't use uninitialized data in case
            // the return code is not checked.
            explicit_bzero(raw_privkey, 64);
        }
        FINALLY {}
    }
    END_TRY;

    return error;
}

/**
 * @brief   Gets the private key from the device seed using the specified bip32 path.
 *
 * @param[in]  curve           Curve identifier.
 *
 * @param[in]  path            Bip32 path to use for derivation.
 *
 * @param[in]  path_len        Bip32 path length.
 *
 * @param[out] raw_privkey     Buffer where to store the private key.
 *
 * @param[out] chain_code      Buffer where to store the chain code. Can be NULL.
 *
 * @return                     Error code:
 *                             - CX_OK on success
 *                             - CX_INTERNAL_ERROR
 */
WARN_UNUSED_RESULT static inline cx_err_t os_derive_bip32_no_throw(
    cx_curve_t          curve,
    const unsigned int *path,
    unsigned int        path_len,
    unsigned char       raw_privkey[static 64],
    unsigned char      *chain_code)
{
    return os_derive_bip32_with_seed_no_throw(
        HDW_NORMAL, curve, path, path_len, raw_privkey, chain_code, NULL, 0);
}

// Deprecated : see "os_derive_eip2333_no_throw"
#ifndef HAVE_BOLOS
DEPRECATED
#endif
SYSCALL void os_perso_derive_eip2333(cx_curve_t                curve,
                                     const unsigned int *path  PLENGTH(4
                                                                      * (pathLength & 0x0FFFFFFFu)),
                                     unsigned int              pathLength,
                                     unsigned char *privateKey PLENGTH(32));

/**
 * @brief   Gets the private key from the device seed using the specified eip2333 path.
 *
 * @param[in]  curve           Curve identifier.
 *
 * @param[in]  path            Eip2333 path to use for derivation.
 *
 * @param[in]  path_len        Eip2333 path length.
 *
 * @param[out] raw_privkey     Buffer where to store the private key.
 *
 * @return                     Error code:
 *                             - CX_OK on success
 *                             - CX_INTERNAL_ERROR
 */
WARN_UNUSED_RESULT static inline cx_err_t os_derive_eip2333_no_throw(
    cx_curve_t          curve,
    const unsigned int *path,
    unsigned int        path_len,
    unsigned char       raw_privkey[static 64])
{
    cx_err_t error = CX_OK;

    BEGIN_TRY
    {
        TRY
        {
// ignore the deprecated warning, pragma to remove when the "no throw" OS function will be available
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
            // Derive the seed with path
            os_perso_derive_eip2333(curve, path, path_len, raw_privkey);
#pragma GCC diagnostic pop
        }
        CATCH_OTHER(e)
        {
            error = e;

            // Make sure the caller doesn't use uninitialized data in case
            // the return code is not checked.
            explicit_bzero(raw_privkey, 64);
        }
        FINALLY {}
    }
    END_TRY;

    return error;
}

SYSCALL bolos_err_t os_perso_get_master_key_identifier(uint8_t *identifier,
                                                       size_t   identifier_length);
