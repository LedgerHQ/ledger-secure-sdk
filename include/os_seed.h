#pragma once

#include <string.h>

#include "appflags.h"
#include "decorators.h"
#include "lcx_ecfp.h"
#include "ox_ec.h"
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

#ifdef HAVE_SECP256K1_CURVE

// Maximum supported number of derivation steps in os_derive_slip21_key
#define SLIP21_MAX_LABELS 10

// Maximum total length of all labels in os_derive_slip21_key
#define SLIP21_MAX_TOTAL_LENGTH 256

/**
 * @brief   Derives a SLIP-21 key from a list of labels.
 *
 * @details At most 10 labels are allowed. Each label must not contain the '/' character and must
 * not be empty. The total length of the labels must not exceed 256 bytes. The derivation must be
 * one of the allowed ones in the application's Makefile.
 *
 * @param[in]  labels          Array of null-terminated label strings.
 *
 * @param[in]  labels_count    Number of labels in the array. At least 1 and most 10 labels are
 * allowed.
 *
 * @param[out] key             Buffer where to store the derived key (32 bytes).
 *
 * @return                     Error code:
 *                             - CX_OK on success
 *                             - CX_INVALID_PARAMETER if any constraint is violated
 *                             - CX_INTERNAL_ERROR
 */
WARN_UNUSED_RESULT static inline cx_err_t os_derive_slip21_key(const char  **labels,
                                                               unsigned int  labels_count,
                                                               unsigned char key[static 32])
{
    // Make sure the caller doesn't use uninitialized data. Will be overwritten on success.
    explicit_bzero(key, 32);

    // Up to 10 labels are supported
    if (labels_count == 0 || labels_count > SLIP21_MAX_LABELS) {
        return CX_INVALID_PARAMETER;
    }

    if (labels == NULL) {
        return CX_INVALID_PARAMETER;
    }

    // Build the SLIP-21 path string
    // Aligned to 4 bytes as this is later passed as a (const unsigned int *) pointer
    // Make sure that the buffer is large enough (account for 1 extra byte per label for the \x00
    // prefix, and the 1 byte for the '/' separator after the first label).
    unsigned char path_buffer[SLIP21_MAX_TOTAL_LENGTH + 2 * SLIP21_MAX_LABELS - 1]
        __attribute__((aligned(4)));
    unsigned int pos = 0;

    for (unsigned int i = 0; i < labels_count; i++) {
        const char *label = labels[i];

        // Check if individual label is NULL
        if (label == NULL) {
            return CX_INVALID_PARAMETER;
        }

        unsigned int label_len = strlen(label);

        // Reject empty labels
        if (label_len == 0) {
            return CX_INVALID_PARAMETER;
        }

        // Check for '/' character
        if (strchr(label, '/') != NULL) {
            return CX_INVALID_PARAMETER;
        }

        // Check if we would exceed buffer size
        // Need space for: potential '/' separator + \x00 + label
        unsigned int needed = (i > 0 ? 1 : 0) + 1 + label_len;  // \x00 prefix + label

        if (pos + needed > sizeof(path_buffer)) {
            return CX_INVALID_PARAMETER;
        }

        // Add separator if not first element
        if (i > 0) {
            path_buffer[pos++] = '/';
        }

        // Add \x00 prefix
        path_buffer[pos++] = 0x00;

        // Copy the label (excluding the null terminator)
        memcpy(&path_buffer[pos], label, label_len);
        pos += label_len;
    }

    // Derive the key using SLIP-21
    unsigned char raw_privkey[64];
    cx_err_t      error = os_derive_bip32_with_seed_no_throw(HDW_SLIP21,
                                                        CX_CURVE_SECP256K1,
                                                        (const unsigned int *) path_buffer,
                                                        pos,
                                                        raw_privkey,
                                                        NULL,
                                                        NULL,
                                                        0);

    if (error == CX_OK) {
        // Return only the first 32 bytes
        memcpy(key, raw_privkey, 32);
    }

    // Clear sensitive data
    explicit_bzero(raw_privkey, sizeof(raw_privkey));

    return error;
}
#endif  // HAVE_SECP256K1_CURVE

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
