/**
 * Stub: check_signature_with_pki always succeeds, allowing fuzzers to reach
 * code paths past signature verification.
 */

#include "ledger_pki.h"
#include "buffer.h"
#include "ox_ec.h"

check_signature_with_pki_status_t check_signature_with_pki(const buffer_t    hash,
                                                           const uint8_t    *expected_key_usage,
                                                           const cx_curve_t *expected_curve,
                                                           const buffer_t    signature)
{
    (void) hash;
    (void) expected_key_usage;
    (void) expected_curve;
    (void) signature;
    return CHECK_SIGNATURE_WITH_PKI_SUCCESS;
}
