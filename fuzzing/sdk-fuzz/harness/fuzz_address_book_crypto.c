/* Direct harness for address_book_crypto.c.
 *
 * Bypasses TLV parsing entirely and exercises all crypto API functions with
 * raw fuzz data.  This eliminates the "input must be valid TLV to reach the
 * crypto layer" bottleneck that kept address_book_crypto.c at 0% coverage.
 *
 * Fuzz buffer layout (fields are read sequentially; missing bytes → 0):
 *   [0]       flags          bit0=hmac_fail, bit1=hmac_verify_fail
 *   [1]       sel            selects which function(s) to call
 *   [2]       path_len_raw   clamped to [1..MAX_BIP32_PATH]
 *   [3..42]   path[10]       ten uint32_t (big-endian pairs of 2 bytes each)
 *   [43..74]  gid[32]
 *   [75]      name_len_raw   clamped to [0..CONTACT_NAME_LENGTH-1]
 *   [76..107] name[32]       null-terminated by harness
 *   [108]     scope_len_raw  clamped to [0..SCOPE_LENGTH-1]
 *   [109..140] scope[32]     null-terminated by harness
 *   [141]     id_len_raw     clamped to [0..IDENTIFIER_MAX_LENGTH]
 *   [142..221] identifier[80]
 *   [222]     family_raw     clamped to [0..FAMILY_COUNT-1]
 *   [223..230] chain_id      big-endian uint64
 *   [231..262] hmac_expected[32]
 *   [263..326] group_handle[64]
 *   [327..358] hmac_proof[32]
 *   [359..390] hmac_rest[32]
 *   [391]     type_byte      for address_book_send_hmac_proof
 *
 * Total worst-case: 392 bytes.  Inputs shorter than this work because the
 * layout struct is zero-initialised before copying.
 */

#include "mocks.h"

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <setjmp.h>

#include "address_book_crypto.h"
#include "address_book.h"
#include "identity.h"
#include "ledger_account.h"
#include "bip32.h"
#include "lcx_sha256.h"
#include "io.h"
#include "buffer.h"
#include "os_address_book.h"
#include "lcx_rng.h"

/* ── Linker stubs ────────────────────────────────────────────────────────────── */

/* os_io_default_apdu.c references addr_book_handle_apdu but this harness only
 * links address_book_crypto.c.  Provide a no-op stub to satisfy the linker. */
#include "address_book.h"
bolos_err_t addr_book_handle_apdu(uint8_t *buffer, size_t buffer_len, uint8_t p1, uint8_t p2)
{
    (void) buffer;
    (void) buffer_len;
    (void) p1;
    (void) p2;
    return 0;
}

/* ── Syscall stubs controlled by fuzz flags ─────────────────────────────────── */

static bool s_hmac_fail        = false;
static bool s_hmac_verify_fail = false;

bool sys_address_book_hmac(const uint32_t        *bip32_path,
                           size_t                 bip32_path_len,
                           ADDRESS_BOOK_salt_id_t salt_id,
                           const uint8_t         *message,
                           size_t                 message_len,
                           uint8_t                hmac_out[CX_SHA256_SIZE])
{
    (void) bip32_path;
    (void) bip32_path_len;
    (void) salt_id;
    (void) message;
    (void) message_len;
    if (s_hmac_fail) {
        return false;
    }
    memset(hmac_out, 0xAB, CX_SHA256_SIZE);
    return true;
}

bool sys_address_book_hmac_verify(const uint32_t        *bip32_path,
                                  size_t                 bip32_path_len,
                                  ADDRESS_BOOK_salt_id_t salt_id,
                                  const uint8_t         *message,
                                  size_t                 message_len,
                                  const uint8_t          hmac_expected[CX_SHA256_SIZE])
{
    (void) bip32_path;
    (void) bip32_path_len;
    (void) salt_id;
    (void) message;
    (void) message_len;
    (void) hmac_expected;
    return !s_hmac_verify_fail;
}

void cx_rng_no_throw(uint8_t *buffer, size_t len)
{
    memset(buffer, 0x42, len);
}

/* ── Fuzz layout ────────────────────────────────────────────────────────────── */

#define PATH_BYTES (MAX_BIP32_PATH * sizeof(uint32_t)) /* 40 bytes */

typedef struct {
    uint8_t flags;
    uint8_t sel;
    uint8_t path_len_raw;
    uint8_t path_bytes[PATH_BYTES]; /* 40 bytes, read as big-endian uint32 pairs */
    uint8_t gid[GID_SIZE];          /* 32 */
    uint8_t name_len_raw;
    char    name[CONTACT_NAME_LENGTH]; /* 33, null-terminated by harness */
    uint8_t scope_len_raw;
    char    scope[SCOPE_LENGTH]; /* 33, null-terminated by harness */
    uint8_t id_len_raw;
    uint8_t identifier[IDENTIFIER_MAX_LENGTH]; /* 80 */
    uint8_t family_raw;
    uint8_t chain_id_bytes[8];
    uint8_t hmac_expected[CX_SHA256_SIZE];   /* 32 */
    uint8_t group_handle[GROUP_HANDLE_SIZE]; /* 64 */
    uint8_t hmac_proof[CX_SHA256_SIZE];      /* 32 */
    uint8_t hmac_rest[CX_SHA256_SIZE];       /* 32 */
    uint8_t type_byte;
} crypto_layout_t;

/* Number of callable variants.  One per public function plus an extra
 * "call several in sequence" mode that exercises the common registration path
 * end-to-end (generate → compute proof → compute rest → send response). */
#define N_VARIANTS 12U

int fuzz_entry(const uint8_t *data, size_t size)
{
    try_context_set(&fuzz_exit_jump_ctx);
    if (sigsetjmp(fuzz_exit_jump_ctx.jmp_buf, 1)) {
        try_context_set(NULL);
        memset(&fuzz_exit_jump_ctx, 0, sizeof(fuzz_exit_jump_ctx));
        return 0;
    }

    /* Overlay fuzz data onto the layout (zero-initialised for short inputs). */
    crypto_layout_t L = {0};
    size_t          n = size < sizeof(L) ? size : sizeof(L);
    memcpy(&L, data, n);

    /* ── Decode flags ── */
    s_hmac_fail        = (L.flags & 0x01) != 0;
    s_hmac_verify_fail = (L.flags & 0x02) != 0;

    /* ── Build BIP32 path ── */
    path_bip32_t bip32 = {0};
    bip32.length       = (L.path_len_raw % MAX_BIP32_PATH) + 1U;
    for (size_t i = 0; i < bip32.length; i++) {
        const uint8_t *b = &L.path_bytes[i * 4];
        bip32.path[i] = ((uint32_t) b[0] << 24) | ((uint32_t) b[1] << 16) | ((uint32_t) b[2] << 8)
                        | (uint32_t) b[3];
    }

    /* ── Decode string lengths and ensure null termination ── */
    size_t nlen                = L.name_len_raw % CONTACT_NAME_LENGTH;
    L.name[nlen]               = '\0';
    size_t slen                = L.scope_len_raw % SCOPE_LENGTH;
    L.scope[slen]              = '\0';
    uint8_t             id_len = L.id_len_raw % (IDENTIFIER_MAX_LENGTH + 1U);
    blockchain_family_e fam    = (blockchain_family_e) (L.family_raw % FAMILY_COUNT);
    uint64_t            chain_id
        = ((uint64_t) L.chain_id_bytes[0] << 56) | ((uint64_t) L.chain_id_bytes[1] << 48)
          | ((uint64_t) L.chain_id_bytes[2] << 40) | ((uint64_t) L.chain_id_bytes[3] << 32)
          | ((uint64_t) L.chain_id_bytes[4] << 24) | ((uint64_t) L.chain_id_bytes[5] << 16)
          | ((uint64_t) L.chain_id_bytes[6] << 8) | (uint64_t) L.chain_id_bytes[7];

    /* ── Output buffers ── */
    uint8_t out32[CX_SHA256_SIZE]     = {0};
    uint8_t gid_out[GID_SIZE]         = {0};
    uint8_t gh_out[GROUP_HANDLE_SIZE] = {0};

    switch (L.sel % N_VARIANTS) {
        /* ── address_book_generate_group_handle ── */
        case 0:
            address_book_generate_group_handle(gh_out);
            break;

        /* ── address_book_verify_group_handle ── */
        case 1:
            address_book_verify_group_handle(L.group_handle, gid_out);
            break;

        /* ── address_book_compute_hmac_proof (identity) ── */
        case 2:
            address_book_compute_hmac_proof(L.gid, L.name, out32);
            break;

        /* ── address_book_verify_hmac_proof (identity) ── */
        case 3:
            address_book_verify_hmac_proof(L.gid, L.name, L.hmac_expected);
            break;

        /* ── address_book_compute_hmac_rest ── */
        case 4:
            address_book_compute_hmac_rest(
                L.gid, L.scope, L.identifier, id_len, fam, chain_id, out32);
            break;

        /* ── address_book_verify_hmac_rest ── */
        case 5:
            address_book_verify_hmac_rest(
                L.gid, L.scope, L.identifier, id_len, fam, chain_id, L.hmac_expected);
            break;

        /* ── address_book_send_hmac_proof ── */
        case 6:
            address_book_send_hmac_proof(L.type_byte, L.hmac_proof);
            break;

        /* ── address_book_send_register_identity_response ── */
        case 7:
            address_book_send_register_identity_response(L.group_handle, L.hmac_proof, L.hmac_rest);
            break;

#ifdef HAVE_ADDRESS_BOOK_LEDGER_ACCOUNT
        /* ── address_book_compute_hmac_proof_ledger_account ── */
        case 8:
            address_book_compute_hmac_proof_ledger_account(&bip32, L.name, fam, chain_id, out32);
            break;

        /* ── address_book_verify_hmac_proof_ledger_account ── */
        case 9:
            address_book_verify_hmac_proof_ledger_account(
                &bip32, L.name, fam, chain_id, L.hmac_expected);
            break;
#endif /* HAVE_ADDRESS_BOOK_LEDGER_ACCOUNT */

        /* ── Full new-group registration path ─────────────────────────────
         * Exercises generate → compute_proof → compute_rest → send_response
         * in sequence, which is the exact happy path in build_and_send_response()
         * when no existing group is provided. */
        case 10: {
            uint8_t group_handle[GROUP_HANDLE_SIZE] = {0};
            uint8_t hmac_proof[CX_SHA256_SIZE]      = {0};
            uint8_t hmac_rest[CX_SHA256_SIZE]       = {0};

            if (!address_book_generate_group_handle(group_handle)) {
                break;
            }
            const uint8_t *gid = group_handle; /* first 32 bytes */
            if (!address_book_compute_hmac_proof(gid, L.name, hmac_proof)) {
                break;
            }
            if (!address_book_compute_hmac_rest(
                    gid, L.scope, L.identifier, id_len, fam, chain_id, hmac_rest)) {
                break;
            }
            address_book_send_register_identity_response(group_handle, hmac_proof, hmac_rest);

            explicit_bzero(group_handle, sizeof(group_handle));
            explicit_bzero(hmac_proof, sizeof(hmac_proof));
            explicit_bzero(hmac_rest, sizeof(hmac_rest));
            break;
        }

        /* ── Existing-group verification path ─────────────────────────────
         * Exercises verify_group_handle → verify_hmac_proof, as done in
         * register_identity() when TAG_GROUP_HANDLE is present. */
        case 11: {
            uint8_t gid_buf[GID_SIZE] = {0};
            if (!address_book_verify_group_handle(L.group_handle, gid_buf)) {
                break;
            }
            address_book_verify_hmac_proof(gid_buf, L.name, L.hmac_proof);
            break;
        }

        default:
            break;
    }

    try_context_set(NULL);
    memset(&fuzz_exit_jump_ctx, 0, sizeof(fuzz_exit_jump_ctx));
    return 0;
}
