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

/**
 * @file transaction_check.h
 * @brief Transaction Check: risk report from a security provider, displayed during the review.
 *
 * The descriptor is received either through the SDK default APDU (CLA 0xB0) or through an
 * app-specific APDU calling tx_check_provide_descriptor() and tx_check_opt_in().
 * The coin app binds the report to the transaction being signed with tx_check_apply_report().
 */

#pragma once

/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "os_types.h"
#include "buffer.h"
#include "lcx_ecdsa.h"
#include "lcx_sha3.h"
#include "nbgl_use_case.h"
#include "tlv_use_case_transaction_check.h"

#if defined(HAVE_TX_CHECK)

/* Exported enumerations -----------------------------------------------------*/
typedef enum {
    TX_CHECK_SETTING_OPT_IN,  ///< The user has seen the opt-in screen
    TX_CHECK_SETTING_ENABLE,  ///< Transaction Check is enabled
} tx_check_setting_t;

/* Exported types, structures, unions ----------------------------------------*/

/** Operation being signed, to which the received report must be bound. */
typedef struct {
    transaction_check_type_t type;
    /** Transaction or message hash (TRANSACTION_CHECK_HASH_SIZE bytes) */
    const uint8_t *tx_hash;
    /** Domain hash (TRANSACTION_CHECK_HASH_SIZE bytes), NULL when not applicable */
    const uint8_t *domain_hash;
    /** Coin-app data forwarded untouched to the entrypoints */
    const void *app_data;
} tx_check_context_t;

/* Exported defines   --------------------------------------------------------*/

/**
 * Largest descriptor, every tag at its max size.
 * Each TLV is tag + length + value; tags from 0x80 are DER-encoded on 2 bytes, lengths on 1 byte.
 */
// clang-format off
#define TRANSACTION_CHECK_MAX_PAYLOAD_SIZE                                       \
    ((1 + 1 + 1)                                       /* STRUCTURE_TYPE      */ \
     + (1 + 1 + 1)                                     /* STRUCTURE_VERSION   */ \
     + (1 + 1 + TRANSACTION_CHECK_MAX_ADDRESS_SIZE)    /* ADDRESS             */ \
     + (1 + 1 + 8)                                     /* CHAIN_ID            */ \
     + (1 + 1 + TRANSACTION_CHECK_HASH_SIZE)           /* TX_HASH             */ \
     + (1 + 1 + TRANSACTION_CHECK_HASH_SIZE)           /* DOMAIN_HASH         */ \
     + (2 + 1 + 1)                                     /* NORMALIZED_RISK     */ \
     + (2 + 1 + 1)                                     /* NORMALIZED_CATEGORY */ \
     + (2 + 1 + TRANSACTION_CHECK_MSG_SIZE)            /* PROVIDER_MSG        */ \
     + (2 + 1 + TRANSACTION_CHECK_URL_SIZE)            /* TINY_URL            */ \
     + (2 + 1 + 1)                                     /* SIMULATION_TYPE     */ \
     + (1 + 1 + CX_ECDSA_SHA256_SIG_MAX_ASN1_LENGTH))  /* DER_SIGNATURE       */
// clang-format on

/* Exported functions prototypes--------------------------------------------- */

/**
 * @brief Handle the SDK default APDU (CLA 0xB0, INS DEFAULT_APDU_INS_TRANSACTION_CHECK).
 *
 * P1 0x00: descriptor, chunked as described in apdu_chunks.h.
 * P1 0x01: opt-in request; replies with 1 byte (enable state), after the user choice when the
 *          opt-in screen is displayed.
 *
 * @param[in]     buffer             APDU data
 * @param[in]     buffer_len         APDU data length
 * @param[in]     p1                 APDU P1
 * @param[in]     p2                 APDU P2
 * @param[out]    buffer_out         Response data
 * @param[in,out] buffer_out_length  Response capacity in, response length out
 * @return Status word, SWO_NO_RESPONSE when the reply is deferred
 */
bolos_err_t tx_check_handle_apdu(const uint8_t *buffer,
                                 size_t         buffer_len,
                                 uint8_t        p1,
                                 uint8_t        p2,
                                 uint8_t       *buffer_out,
                                 size_t        *buffer_out_length);

/**
 * @brief Parse, verify and store a Transaction Check descriptor.
 *
 * @param[in] payload Complete TLV descriptor
 * @return SWO_SUCCESS,
 *         SWO_COMMAND_CODE_NOT_SUPPORTED if Transaction Check is disabled,
 *         SWO_COMMAND_NOT_ALLOWED if a descriptor is already stored,
 *         SWO_SECURITY_CONDITION_NOT_SATISFIED if the PKI signature is invalid,
 *         SWO_INCORRECT_DATA if the descriptor is invalid
 */
uint16_t tx_check_provide_descriptor(const buffer_t *payload);

/**
 * @brief Display the opt-in screen if the user has not seen it yet.
 *
 * When displayed and @p response_expected is set, the 1-byte enable state is sent as the APDU
 * reply once the user has chosen.
 *
 * @param[in] response_expected Whether an APDU reply is pending
 * @return true if the opt-in screen is displayed, false if the user has already opted in
 */
bool tx_check_opt_in(bool response_expected);

/**
 * @brief Bind the stored report to the operation being signed and fill the review warning.
 *
 * Clears the W3C warnings and the report fields of @p warning, then, when Transaction Check is
 * enabled, sets W3C_ISSUE_WARN when no report matches the operation, otherwise the W3C warning of
 * the report risk level. Typed data must be given with its domain hash.
 *
 * @param[in,out] warning Review warning, other bits are kept
 * @param[in]     ctx     Operation being signed
 */
void tx_check_apply_report(nbgl_warning_t *warning, const tx_check_context_t *ctx);

/**
 * @brief Erase the stored report.
 */
void tx_check_clear(void);

#endif  // HAVE_TX_CHECK
