/*
 * PKI certificate gate, under fuzzer control.
 *
 * The real check_signature_with_pki() (lib_pki/ledger_pki.c) verifies a
 * certificate before an app accepts a descriptor. In a fuzz build the syscall
 * mocks return defaults, so it always fails with MISSING_CERTIFICATE and
 * everything behind the gate is unreachable. This overrides it.
 *
 * Returning success is memory-safe: the gate is pure validation and neither
 * copies nor arithmetically combines the fuzzer's bytes, so nothing downstream
 * sees data it would not otherwise see.
 *
 * The verdict is a fuzzable global rather than a constant, so the invariant can
 * drive the rejection branch too. Constrain it in
 * invariants/domain-overrides.txt:
 *
 *     fuzz_mock_pki_fail. = values \x00 \x01
 *
 * 0 = accept, matching the fuzz_mock_nbgl_reject / fuzz_mock_crypto_fail idiom
 * where zero is always the happy path.
 */

#include <stdint.h>

#include "ledger_pki.h"
#include "buffer.h"
#include "ox_ec.h"

/* Absolution discovers this BSS global; apps constrain it in domain-overrides. */
uint8_t fuzz_mock_pki_fail;

check_signature_with_pki_status_t __wrap_check_signature_with_pki(const buffer_t hash,
                                                                  const uint8_t *expected_key_usage,
                                                                  const cx_curve_t *expected_curve,
                                                                  const buffer_t    signature)
{
    (void) hash;
    (void) expected_key_usage;
    (void) expected_curve;
    (void) signature;

    if (fuzz_mock_pki_fail != 0) {
        return CHECK_SIGNATURE_WITH_PKI_MISSING_CERTIFICATE;
    }
    return CHECK_SIGNATURE_WITH_PKI_SUCCESS;
}
