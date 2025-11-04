#pragma once

#include "buffer.h"
#include "cx.h"

typedef enum check_signature_with_pki_status_e {
    CHECK_SIGNATURE_WITH_PKI_SUCCESS                 = 0,
    CHECK_SIGNATURE_WITH_PKI_MISSING_CERTIFICATE     = 1,
    CHECK_SIGNATURE_WITH_PKI_WRONG_CERTIFICATE_USAGE = 2,
    CHECK_SIGNATURE_WITH_PKI_WRONG_CERTIFICATE_CURVE = 3,
    CHECK_SIGNATURE_WITH_PKI_WRONG_SIGNATURE         = 4,
} check_signature_with_pki_status_t;

/**
 * @brief Checks a signature using the PKI certificate.
 *
 * This function retrieves the PKI certificate information from the OS, verifies that the key usage
 * and curve match the expected values, and then checks the signature against the provided hash.
 *
 * @param hash                Buffer containing the hash to verify.
 * @param expected_key_usage  Expected key usage value of the loaded certificate or NULL.
 * @param expected_curve      Expected elliptic curve of the loaded certificate or NULL.
 * @param signature           Buffer containing the signature to verify.
 *
 * @return Status of the signature check (see check_signature_with_pki_status_t).
 */
check_signature_with_pki_status_t check_signature_with_pki(const buffer_t    hash,
                                                           const uint8_t    *expected_key_usage,
                                                           const cx_curve_t *expected_curve,
                                                           const buffer_t    signature);
