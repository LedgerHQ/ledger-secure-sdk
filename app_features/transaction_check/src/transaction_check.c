/*****************************************************************************
 *   (c) 2026 Ledger SAS.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *****************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "os_helpers.h"
#include "os_print.h"
#include "os_utils.h"
#include "status_words.h"
#include "apdu_chunks.h"
#include "transaction_check.h"
#include "transaction_check_entrypoints.h"

#if defined(HAVE_TX_CHECK)

/* Private defines------------------------------------------------------------*/
#define P1_DESCRIPTOR 0x00
#define P1_OPT_IN     0x01

/** Warning bits set by the Transaction Check. */
#define TX_CHECK_WARNINGS                                                                    \
    ((1 << W3C_ISSUE_WARN) | (1 << W3C_RISK_DETECTED_WARN) | (1 << W3C_THREAT_DETECTED_WARN) \
     | (1 << W3C_NO_THREAT_WARN))

/* Private types, structures, unions -----------------------------------------*/
typedef struct {
    bool                         received;
    bool                         chain_id_received;
    bool                         domain_hash_received;
    uint64_t                     chain_id;
    uint8_t                      tx_hash[TRANSACTION_CHECK_HASH_SIZE];
    uint8_t                      domain_hash[TRANSACTION_CHECK_HASH_SIZE];
    uint8_t                      address[TRANSACTION_CHECK_MAX_ADDRESS_SIZE];
    uint8_t                      address_len;
    char                         tiny_url[TRANSACTION_CHECK_URL_SIZE + 1];
    char                         partner[TRANSACTION_CHECK_PARTNER_SIZE + 1];
    transaction_check_risk_t     risk;
    transaction_check_type_t     type;
    transaction_check_category_t category;
} tx_check_report_t;

/* Private variables ---------------------------------------------------------*/
static tx_check_report_t s_report;

/* Private functions ---------------------------------------------------------*/

/**
 * @brief Check that the stored report applies to the operation being signed.
 */
static bool report_matches(const tx_check_context_t *ctx)
{
    if (!s_report.received) {
        PRINTF("[TX CHECK] No report received\n");
        return false;
    }
    if ((ctx == NULL) || (ctx->tx_hash == NULL)) {
        PRINTF("[TX CHECK] No operation to bind the report to\n");
        return false;
    }
    if (s_report.type != ctx->type) {
        PRINTF("[TX CHECK] Type mismatch: %d != %d\n", s_report.type, ctx->type);
        return false;
    }
    if (!handle_check_tx_report_chain_id(s_report.chain_id_received, s_report.chain_id, ctx)) {
        PRINTF("[TX CHECK] Chain ID rejected\n");
        return false;
    }
    if (!handle_check_tx_report_signer(s_report.address, s_report.address_len, ctx)) {
        PRINTF("[TX CHECK] Signer address mismatch\n");
        return false;
    }
    if (memcmp(s_report.tx_hash, ctx->tx_hash, TRANSACTION_CHECK_HASH_SIZE) != 0) {
        PRINTF("[TX CHECK] TX_HASH mismatch: %.*H != %.*H\n",
               TRANSACTION_CHECK_HASH_SIZE,
               s_report.tx_hash,
               TRANSACTION_CHECK_HASH_SIZE,
               ctx->tx_hash);
        return false;
    }
    // Typed data is bound to its domain
    if ((ctx->type == TRANSACTION_CHECK_TYPE_TYPED_DATA) && (ctx->domain_hash == NULL)) {
        PRINTF("[TX CHECK] No domain to bind the typed data report to\n");
        return false;
    }
    if (ctx->domain_hash != NULL) {
        if (!s_report.domain_hash_received
            || (memcmp(s_report.domain_hash, ctx->domain_hash, TRANSACTION_CHECK_HASH_SIZE) != 0)) {
            PRINTF("[TX CHECK] DOMAIN_HASH mismatch\n");
            return false;
        }
    }
    return true;
}

/**
 * @brief Text displayed in the warning, from the report risk and category.
 */
static const char *warning_text(void)
{
    switch (s_report.risk) {
        case TRANSACTION_CHECK_RISK_WARNING:
            switch (s_report.category) {
                case TRANSACTION_CHECK_CATEGORY_ADDRESS:
                    return "This transaction involves a suspicious address. "
                           "It might not be safe to continue.";
                case TRANSACTION_CHECK_CATEGORY_DAPP:
                    return "This transaction involves a suspicious dApp. "
                           "It might not be safe to continue.";
                case TRANSACTION_CHECK_CATEGORY_LOSING_OPERATION:
                    return "This transaction could end in a loss. "
                           "Check transaction details carefully before signing.";
                default:
                    return "This transaction might be malicious. It might not be safe to "
                           "continue. Tap the QR code icon for more details.";
            }
        case TRANSACTION_CHECK_RISK_MALICIOUS:
            switch (s_report.category) {
                case TRANSACTION_CHECK_CATEGORY_ADDRESS:
                    return "This transaction involves a malicious address. "
                           "Your assets will most likely be stolen.";
                case TRANSACTION_CHECK_CATEGORY_DAPP:
                    return "This dApp is linked to a scammer. "
                           "Your assets will most likely be stolen.";
                default:
                    return "This request is malicious. Your assets will most likely be stolen. "
                           "View full report for details.";
            }
        case TRANSACTION_CHECK_RISK_BENIGN:
            // No warning screen for the benign level
            return NULL;
        default:
            return NULL;
    }
}

/* Exported functions --------------------------------------------------------*/
void tx_check_clear(void)
{
    explicit_bzero(&s_report, sizeof(s_report));
}

uint16_t tx_check_provide_descriptor(const buffer_t *payload)
{
    tlv_transaction_check_out_t out = {0};

    if (!handle_tx_check_get_setting(TX_CHECK_SETTING_ENABLE)) {
        PRINTF("[TX CHECK] Transaction Check is disabled\n");
        return SWO_COMMAND_CODE_NOT_SUPPORTED;
    }
    // A report already bound to a review on screen must not be replaced
    if (s_report.received) {
        PRINTF("[TX CHECK] Report already received\n");
        return SWO_COMMAND_NOT_ALLOWED;
    }

    tlv_transaction_check_status_t status = tlv_use_case_transaction_check(payload, &out);
    if (status & TLV_TRANSACTION_CHECK_SIGNATURE_ERROR) {
        PRINTF("[TX CHECK] Descriptor signature rejected with status 0x%x\n", status);
        return SWO_SECURITY_CONDITION_NOT_SATISFIED;
    }
    if (status != TLV_TRANSACTION_CHECK_SUCCESS) {
        PRINTF("[TX CHECK] Descriptor parsing failed with status %d\n", status);
        return SWO_INCORRECT_DATA;
    }
    if (out.additional_data_received) {
        PRINTF("[TX CHECK] Unsupported ADDITIONAL_DATA tag\n");
        return SWO_INCORRECT_DATA;
    }
    if (out.address.size > sizeof(s_report.address)) {
        PRINTF("[TX CHECK] Address too long: %u\n", out.address.size);
        return SWO_INCORRECT_DATA;
    }
    if (!handle_check_tx_report(&out)) {
        PRINTF("[TX CHECK] Descriptor rejected by the app\n");
        return SWO_INCORRECT_DATA;
    }

    s_report.chain_id_received    = out.chain_id_received;
    s_report.chain_id             = out.chain_id;
    s_report.domain_hash_received = out.domain_hash_received;
    memmove(s_report.tx_hash, out.tx_hash.ptr, TRANSACTION_CHECK_HASH_SIZE);
    if (out.domain_hash_received) {
        memmove(s_report.domain_hash, out.domain_hash.ptr, TRANSACTION_CHECK_HASH_SIZE);
    }
    memmove(s_report.address, out.address.ptr, out.address.size);
    s_report.address_len = (uint8_t) out.address.size;
    strlcpy(s_report.tiny_url, out.tiny_url, sizeof(s_report.tiny_url));
    strlcpy(s_report.partner, out.partner, sizeof(s_report.partner));
    s_report.risk     = out.risk;
    s_report.type     = out.type;
    s_report.category = out.category;
    s_report.received = true;

    PRINTF("[TX CHECK] Report: partner=%s risk=%d category=%d type=%d url=%s\n",
           s_report.partner,
           s_report.risk,
           s_report.category,
           s_report.type,
           s_report.tiny_url);
    return SWO_SUCCESS;
}

void tx_check_apply_report(nbgl_warning_t *warning, const tx_check_context_t *ctx)
{
    // Only the result of this call may be shown, whatever the warning held before
    warning->predefinedSet &= ~TX_CHECK_WARNINGS;
    warning->reportProvider  = NULL;
    warning->providerMessage = NULL;
    warning->reportUrl       = NULL;

    if (!handle_tx_check_get_setting(TX_CHECK_SETTING_ENABLE)) {
        PRINTF("[TX CHECK] Transaction Check is disabled\n");
        return;
    }
    if (!report_matches(ctx)) {
        warning->predefinedSet |= (1 << W3C_ISSUE_WARN);
        return;
    }

    switch (s_report.risk) {
        case TRANSACTION_CHECK_RISK_BENIGN:
            warning->predefinedSet |= (1 << W3C_NO_THREAT_WARN);
            break;
        case TRANSACTION_CHECK_RISK_WARNING:
            warning->predefinedSet |= (1 << W3C_RISK_DETECTED_WARN);
            break;
        case TRANSACTION_CHECK_RISK_MALICIOUS:
            warning->predefinedSet |= (1 << W3C_THREAT_DETECTED_WARN);
            break;
        default:
            // Out-of-range values are rejected at reception
            warning->predefinedSet |= (1 << W3C_ISSUE_WARN);
            return;
    }
    warning->reportProvider  = s_report.partner;
    warning->providerMessage = warning_text();
    warning->reportUrl       = s_report.tiny_url;
}

bolos_err_t tx_check_handle_apdu(const uint8_t *buffer,
                                 size_t         buffer_len,
                                 uint8_t        p1,
                                 uint8_t        p2,
                                 uint8_t       *buffer_out,
                                 size_t        *buffer_out_length)
{
    size_t   out_capacity = *buffer_out_length;
    uint8_t *payload      = NULL;
    size_t   payload_len  = 0;

    *buffer_out_length = 0;
    switch (p1) {
        case P1_DESCRIPTOR:
            switch (apdu_chunks_reassemble(APDU_CHUNKS_OWNER_TRANSACTION_CHECK,
                                           buffer,
                                           buffer_len,
                                           p2,
                                           &payload,
                                           &payload_len)) {
                case APDU_CHUNKS_ERROR:
                    return SWO_INCORRECT_DATA;
                case APDU_CHUNKS_PENDING:
                    return SWO_SUCCESS;
                case APDU_CHUNKS_COMPLETE:
                    break;
            }
            return tx_check_provide_descriptor(
                &(const buffer_t){.ptr = payload, .size = payload_len, .offset = 0});

        case P1_OPT_IN:
            if ((p2 != 0) || (buffer_len != 0)) {
                return SWO_WRONG_P1_P2;
            }
            // The opt-in screen would replace a flow on screen
            if (!handle_tx_check_is_idle()) {
                return SWO_COMMAND_NOT_ALLOWED;
            }
            if (tx_check_opt_in(true)) {
                return SWO_NO_RESPONSE;
            }
            if (out_capacity < 1) {
                return SWO_INCORRECT_DATA;
            }
            buffer_out[0]      = handle_tx_check_get_setting(TX_CHECK_SETTING_ENABLE);
            *buffer_out_length = 1;
            return SWO_SUCCESS;

        default:
            PRINTF("[TX CHECK] Unexpected P1 0x%02x\n", p1);
            return SWO_WRONG_P1_P2;
    }
}
#endif  // HAVE_TX_CHECK
