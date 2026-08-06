/* Absolution dispatcher for addr_book_handle_apdu (all sub-commands). */

#include "mocks.h"
#include "parser.h"

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "address_book.h"
#include "address_book_common.h"
#include "address_book_entrypoints.h"
#include "os_address_book.h"
#include "buffer.h"
#include "lcx_rng.h"
#include "tlv_mutator.h"

/* This harness supplies its own LLVMFuzzerCustomMutator() below. */
#define FUZZ_APP_CUSTOM_MUTATOR
#include "fuzz_mutator.h"

/* ── TLV grammar tables (one per P1 sub-command) ────────────────────────── */
/* Used by the custom mutator to produce well-formed TLV inputs.  Each table
 * lists every tag that may appear in the command's TLV payload; the mutator
 * randomly includes or skips each tag, giving coverage of both mandatory-only
 * and optional-field paths.                                                  */

#define N_TAGS(arr) (sizeof(arr) / sizeof((arr)[0]))

/* P1=0x01 — Register Identity */
static const tlv_tag_info_t REGISTER_IDENTITY_TAGS[] = {
    {0x01, 1,  1 }, /* TAG_STRUCTURE_TYPE     */
    {0x02, 1,  1 }, /* TAG_STRUCTURE_VERSION  */
    {0xf0, 1,  32}, /* TAG_CONTACT_NAME       */
    {0xf1, 1,  32}, /* TAG_SCOPE              */
    {0xf2, 1,  80}, /* TAG_IDENTIFIER         */
    {0x69, 5,  41}, /* TAG_DERIVATION_PATH    */
    {0x23, 1,  8 }, /* TAG_CHAIN_ID (ETH)     */
    {0x51, 1,  1 }, /* TAG_BLOCKCHAIN_FAMILY  */
    {0xf6, 64, 64}, /* TAG_GROUP_HANDLE (opt) */
    {0x29, 32, 32}, /* TAG_HMAC_PROOF (opt)   */
};

/* P1=0x02 — Edit Contact Name */
static const tlv_tag_info_t EDIT_CONTACT_NAME_TAGS[] = {
    {0x01, 1,  1 }, /* TAG_STRUCTURE_TYPE    */
    {0x02, 1,  1 }, /* TAG_STRUCTURE_VERSION */
    {0xf0, 1,  32}, /* TAG_CONTACT_NAME      */
    {0xf3, 1,  32}, /* TAG_PREV_NAME         */
    {0xf6, 64, 64}, /* TAG_GROUP_HANDLE      */
    {0x69, 5,  41}, /* TAG_DERIVATION_PATH   */
    {0x29, 32, 32}, /* TAG_HMAC_PROOF        */
};

/* P1=0x03 — Edit Identifier */
static const tlv_tag_info_t EDIT_IDENTIFIER_TAGS[] = {
    {0x01, 1,  1 }, /* TAG_STRUCTURE_TYPE    */
    {0x02, 1,  1 }, /* TAG_STRUCTURE_VERSION */
    {0xf0, 1,  32}, /* TAG_CONTACT_NAME      */
    {0xf1, 1,  32}, /* TAG_SCOPE             */
    {0xf2, 1,  80}, /* TAG_IDENTIFIER        */
    {0xf4, 1,  80}, /* TAG_PREV_IDENTIFIER   */
    {0xf6, 64, 64}, /* TAG_GROUP_HANDLE      */
    {0x69, 5,  41}, /* TAG_DERIVATION_PATH   */
    {0x51, 1,  1 }, /* TAG_BLOCKCHAIN_FAMILY */
    {0x23, 1,  8 }, /* TAG_CHAIN_ID (ETH)    */
    {0x29, 32, 32}, /* TAG_HMAC_PROOF        */
    {0xf7, 32, 32}, /* TAG_HMAC_REST         */
};

/* P1=0x04 — Edit Scope */
static const tlv_tag_info_t EDIT_SCOPE_TAGS[] = {
    {0x01, 1,  1 }, /* TAG_STRUCTURE_TYPE    */
    {0x02, 1,  1 }, /* TAG_STRUCTURE_VERSION */
    {0xf0, 1,  32}, /* TAG_CONTACT_NAME      */
    {0xf1, 1,  32}, /* TAG_SCOPE             */
    {0xf2, 1,  80}, /* TAG_IDENTIFIER        */
    {0xf5, 1,  32}, /* TAG_PREV_SCOPE        */
    {0xf6, 64, 64}, /* TAG_GROUP_HANDLE      */
    {0x69, 5,  41}, /* TAG_DERIVATION_PATH   */
    {0x51, 1,  1 }, /* TAG_BLOCKCHAIN_FAMILY */
    {0x23, 1,  8 }, /* TAG_CHAIN_ID (ETH)    */
    {0x29, 32, 32}, /* TAG_HMAC_PROOF        */
    {0xf7, 32, 32}, /* TAG_HMAC_REST         */
};

/* P1=0x11 — Register Ledger Account */
static const tlv_tag_info_t REGISTER_LEDGER_ACCOUNT_TAGS[] = {
    {0x01, 1, 1 }, /* TAG_STRUCTURE_TYPE    */
    {0x02, 1, 1 }, /* TAG_STRUCTURE_VERSION */
    {0xf0, 1, 32}, /* TAG_CONTACT_NAME      */
    {0x69, 5, 41}, /* TAG_DERIVATION_PATH   */
    {0x51, 1, 1 }, /* TAG_BLOCKCHAIN_FAMILY */
    {0x23, 1, 8 }, /* TAG_CHAIN_ID (ETH)    */
};

/* P1=0x12 — Edit Ledger Account */
static const tlv_tag_info_t EDIT_LEDGER_ACCOUNT_TAGS[] = {
    {0x01, 1,  1 }, /* TAG_STRUCTURE_TYPE    */
    {0x02, 1,  1 }, /* TAG_STRUCTURE_VERSION */
    {0xf0, 1,  32}, /* TAG_CONTACT_NAME      */
    {0xf3, 1,  32}, /* TAG_PREV_NAME         */
    {0x69, 5,  41}, /* TAG_DERIVATION_PATH   */
    {0x51, 1,  1 }, /* TAG_BLOCKCHAIN_FAMILY */
    {0x23, 1,  8 }, /* TAG_CHAIN_ID (ETH)    */
    {0x29, 32, 32}, /* TAG_HMAC_PROOF        */
};

/* P1=0x20 — Provide Contact */
static const tlv_tag_info_t PROVIDE_CONTACT_TAGS[] = {
    {0x01, 1,  1 }, /* TAG_STRUCTURE_TYPE    */
    {0x02, 1,  1 }, /* TAG_STRUCTURE_VERSION */
    {0xf0, 1,  32}, /* TAG_CONTACT_NAME      */
    {0xf1, 1,  32}, /* TAG_SCOPE             */
    {0xf2, 1,  80}, /* TAG_IDENTIFIER        */
    {0xf6, 64, 64}, /* TAG_GROUP_HANDLE      */
    {0x69, 5,  41}, /* TAG_DERIVATION_PATH   */
    {0x51, 1,  1 }, /* TAG_BLOCKCHAIN_FAMILY */
    {0x23, 1,  8 }, /* TAG_CHAIN_ID (ETH)    */
    {0x29, 32, 32}, /* TAG_HMAC_PROOF        */
    {0xf7, 32, 32}, /* TAG_HMAC_REST         */
};

/* P1=0x21 — Provide Ledger Account Contact */
static const tlv_tag_info_t PROVIDE_LEDGER_ACCOUNT_TAGS[] = {
    {0x01, 1,  1 }, /* TAG_STRUCTURE_TYPE    */
    {0x02, 1,  1 }, /* TAG_STRUCTURE_VERSION */
    {0xf0, 1,  32}, /* TAG_CONTACT_NAME      */
    {0x69, 5,  41}, /* TAG_DERIVATION_PATH   */
    {0x51, 1,  1 }, /* TAG_BLOCKCHAIN_FAMILY */
    {0x23, 1,  8 }, /* TAG_CHAIN_ID (ETH)    */
    {0x29, 32, 32}, /* TAG_HMAC_PROOF        */
};

/* ── P1-aware custom mutator ─────────────────────────────────────────────── */
/* Harness input: [ctrl][cmd][p1][p2][TLV payload...]. The framework hands this
 * function the input with the sampled prefix already stripped, so only the four
 * control bytes are stepped over here. They are preserved so the grammar picked
 * from P1 stays consistent with the P1 the harness will dispatch.            */
static size_t address_book_mutate_input(uint8_t     *input,
                                        size_t       size,
                                        size_t       max_size,
                                        unsigned int seed)
{
    if (size <= FUZZ_CTRL_LEN || max_size <= FUZZ_CTRL_LEN) {
        return size;
    }

    /* Select grammar based on the raw P1 byte; harness will clamp it. */
    uint8_t               p1     = input[2];
    const tlv_tag_info_t *tags   = NULL;
    size_t                n_tags = 0;

    switch (p1) {
        case 0x01:
            tags   = REGISTER_IDENTITY_TAGS;
            n_tags = N_TAGS(REGISTER_IDENTITY_TAGS);
            break;
        case 0x02:
            tags   = EDIT_CONTACT_NAME_TAGS;
            n_tags = N_TAGS(EDIT_CONTACT_NAME_TAGS);
            break;
        case 0x03:
            tags   = EDIT_IDENTIFIER_TAGS;
            n_tags = N_TAGS(EDIT_IDENTIFIER_TAGS);
            break;
        case 0x04:
            tags   = EDIT_SCOPE_TAGS;
            n_tags = N_TAGS(EDIT_SCOPE_TAGS);
            break;
        case 0x11:
            tags   = REGISTER_LEDGER_ACCOUNT_TAGS;
            n_tags = N_TAGS(REGISTER_LEDGER_ACCOUNT_TAGS);
            break;
        case 0x12:
            tags   = EDIT_LEDGER_ACCOUNT_TAGS;
            n_tags = N_TAGS(EDIT_LEDGER_ACCOUNT_TAGS);
            break;
        case 0x20:
            tags   = PROVIDE_CONTACT_TAGS;
            n_tags = N_TAGS(PROVIDE_CONTACT_TAGS);
            break;
        case 0x21:
            tags   = PROVIDE_LEDGER_ACCOUNT_TAGS;
            n_tags = N_TAGS(PROVIDE_LEDGER_ACCOUNT_TAGS);
            break;
        default:
            return size;
    }

    current_tlv_fuzz_config.tags_info = tags;
    current_tlv_fuzz_config.num_tags  = n_tags;

    /* Mutate only the TLV region; ctrl/cmd/p1/p2 are preserved. */
    size_t tlv_size = tlv_custom_mutate(
        input + FUZZ_CTRL_LEN, size - FUZZ_CTRL_LEN, max_size - FUZZ_CTRL_LEN, seed);

    return FUZZ_CTRL_LEN + tlv_size;
}

size_t LLVMFuzzerCustomMutator(uint8_t *data, size_t size, size_t max_size, unsigned int seed)
{
    if ((seed & 1U) != 0) {
        return fuzz_custom_mutator(data, size, max_size, seed);
    }

    return fuzz_mutate_input_with(data, size, max_size, seed >> 1, address_book_mutate_input);
}

/* ── OS syscall stubs ────────────────────────────────────────────────────── */

/* HMAC always succeeds and produces a deterministic 32-byte output of 0xAB. */
bool sys_address_book_hmac(const uint32_t        *bip32_path,
                           size_t                 bip32_path_len,
                           ADDRESS_BOOK_salt_id_t salt_id,
                           const uint8_t         *message,
                           size_t                 message_len,
                           uint8_t                hmac_out[32])
{
    (void) bip32_path;
    (void) bip32_path_len;
    (void) salt_id;
    (void) message;
    (void) message_len;
    memset(hmac_out, 0xAB, 32);
    return true;
}

/* HMAC verify always succeeds, enabling the happy path through all flows. */
bool sys_address_book_hmac_verify(const uint32_t        *bip32_path,
                                  size_t                 bip32_path_len,
                                  ADDRESS_BOOK_salt_id_t salt_id,
                                  const uint8_t         *message,
                                  size_t                 message_len,
                                  const uint8_t          hmac_expected[32])
{
    (void) bip32_path;
    (void) bip32_path_len;
    (void) salt_id;
    (void) message;
    (void) message_len;
    (void) hmac_expected;
    return true;
}

void cx_rng_no_throw(uint8_t *buffer, size_t len)
{
    memset(buffer, 0x42, len);
}

/* ── App entrypoint stubs ────────────────────────────────────────────────── */

bool handle_check_register_identity(identity_t *params)
{
    (void) params;
    return true;
}

void finalize_ui_register_identity(void) {}

nbgl_contentTagValue_t *get_register_identity_tagValue(uint8_t pairIndex)
{
    (void) pairIndex;
    return NULL;
}

void finalize_ui_edit_contact_name(void) {}

void on_edit_contact_name_applied(const edit_contact_name_t *edit)
{
    (void) edit;
}

bool handle_check_edit_identifier(const edit_identifier_t *params)
{
    (void) params;
    return true;
}

nbgl_contentTagValue_t *get_edit_identifier_tagValue(uint8_t pairIndex)
{
    (void) pairIndex;
    return NULL;
}

void finalize_ui_edit_identifier(void) {}

void on_edit_identifier_applied(const edit_identifier_t *edit)
{
    (void) edit;
}

void finalize_ui_edit_scope(void) {}

void on_edit_scope_applied(const edit_scope_t *edit)
{
    (void) edit;
}

bool handle_provide_identity(const identity_t *contact)
{
    (void) contact;
    return true;
}

#ifdef HAVE_ADDRESS_BOOK_LEDGER_ACCOUNT

bool handle_check_register_ledger_account(ledger_account_t *params)
{
    (void) params;
    return true;
}

void display_register_ledger_account_review(nbgl_choiceCallback_t choice_callback)
{
    if (choice_callback) {
        choice_callback(true);
    }
}

void finalize_ui_ledger_account(void) {}

bool handle_check_edit_ledger_account(edit_ledger_account_t *params)
{
    (void) params;
    return true;
}

void on_edit_ledger_account_applied(const edit_ledger_account_t *edit)
{
    (void) edit;
}

bool handle_provide_ledger_account(const ledger_account_t *account)
{
    (void) account;
    return true;
}

#endif /* HAVE_ADDRESS_BOOK_LEDGER_ACCOUNT */

/* ── Fuzz harness ────────────────────────────────────────────────────────── */

#include "fuzz_harness.h"

/* All valid P1 values (0x01–0x04, 0x11, 0x12, 0x20, 0x21), covered by clamping
 * to p1_max=0x21.
 * P2=0x00 → single first chunk (header declares exact payload length).
 * P2=0x80 → two-chunk split: first chunk header declares the full length but
 *            delivers only the first half (deliberately inconsistent chunk size),
 *            which forces REASSEMBLY_PENDING; the second half follows as a
 *            P2=0x80 continuation.  This exercises the multi-chunk reassembly
 *            path, including the PENDING→COMPLETE transition and the overflow /
 *            underflow error paths when the declared total mismatches. */
const fuzz_command_spec_t fuzz_commands[] = {
    {.cla = 0x00, .ins = 0x00, .p1_max = 0x21, .p2_max = 0x80},
};
FUZZ_COMMAND_COUNT();

/* Maximum payload per APDU chunk (mirrors address_book.c). */
#define AB_MAX_CHUNK (OS_IO_SEPH_BUFFER_SIZE - 3 - 5)

void fuzz_app_reset(void)
{
    /* Reset shared payload and UI state. */
    memset(&g_ab_payload, 0, sizeof(g_ab_payload));
    memset(&g_ab_ui, 0, sizeof(g_ab_ui));

    /* Reset the static reassembly state inside address_book.c by sending a
     * first-chunk with total_len=0, which triggers REASSEMBLY_ERROR and
     * clears s_chunk_total. */
    uint8_t dummy[2] = {0x00, 0x00};
    addr_book_handle_apdu(dummy, sizeof(dummy), 0x01, 0x00);
}

void fuzz_app_dispatch(void *cmd)
{
    const command_t *c = (const command_t *) cmd;
    if (!fuzz_tail_ptr || fuzz_tail_len == 0) {
        return;
    }

    if (c->p2 == 0x00) {
        /* Single first-chunk: 2-byte BE total-length header + full payload. */
        size_t  payload_len = fuzz_tail_len > AB_MAX_CHUNK ? AB_MAX_CHUNK : fuzz_tail_len;
        uint8_t buf[2 + AB_MAX_CHUNK];
        buf[0] = (uint8_t) (payload_len >> 8);
        buf[1] = (uint8_t) (payload_len & 0xFF);
        memcpy(buf + 2, fuzz_tail_ptr, payload_len);
        addr_book_handle_apdu(buf, 2 + payload_len, c->p1, 0x00);
    }
    else {
        /* Two-chunk split.
         * First chunk: header declares total but delivers only first half —
         * the mismatch between declared total and chunk size forces
         * REASSEMBLY_PENDING and exercises the continuation path.
         * Continuation: delivers the second half (may trigger COMPLETE or
         * an overflow error if the declared total was already exceeded). */
        size_t total     = fuzz_tail_len > AB_MAX_CHUNK ? AB_MAX_CHUNK : fuzz_tail_len;
        size_t first_len = total / 2;
        size_t cont_len  = total - first_len;

        uint8_t first_buf[2 + AB_MAX_CHUNK];
        first_buf[0] = (uint8_t) (total >> 8);
        first_buf[1] = (uint8_t) (total & 0xFF);
        memcpy(first_buf + 2, fuzz_tail_ptr, first_len);
        addr_book_handle_apdu(first_buf, 2 + first_len, c->p1, 0x00);

        if (cont_len > 0) {
            uint8_t cont_buf[AB_MAX_CHUNK];
            memcpy(cont_buf, fuzz_tail_ptr + first_len, cont_len);
            addr_book_handle_apdu(cont_buf, cont_len, c->p1, 0x80);
        }
    }
}
