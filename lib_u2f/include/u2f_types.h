/*****************************************************************************
 *   (c) 2025 Ledger SAS.
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

#pragma once

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

/* Exported enumerations -----------------------------------------------------*/

/* Exported types, structures, unions ----------------------------------------*/
typedef enum {
    U2F_STATE_IDLE                  = 0x00,
    U2F_STATE_CMD_FRAMING           = 0x01,
    U2F_STATE_CMD_COMPLETE          = 0x02,
    U2F_STATE_CMD_PROCESSING        = 0x03,
    U2F_STATE_CMD_PROCESSING_CANCEL = 0x04,
} u2f_state_t;

typedef enum {
    // Common
    U2F_COMMAND_PING  = 0x01,
    U2F_COMMAND_MSG   = 0x03,
    U2F_COMMAND_ERROR = 0x3F,

    // USB HID
    U2F_COMMAND_HID_LOCK       = 0x04,
    U2F_COMMAND_HID_INIT       = 0x06,
    U2F_COMMAND_HID_WINK       = 0x08,
    U2F_COMMAND_HID_CBOR       = 0x10,
    U2F_COMMAND_HID_CANCEL     = 0x11,
    U2F_COMMAND_HID_KEEP_ALIVE = 0x3B,

    // BLE
    U2F_COMMAND_BLE_KEEP_ALIVE = 0x02,
    U2F_COMMAND_BLE_CANCEL     = 0x3E,

} u2f_command_t;

typedef enum {
    U2F_KEEP_ALIVE_REASON_PROCESSING = 0x01,
    U2F_KEEP_ALIVE_REASON_UP_NEEDED  = 0x02,
} u2f_keep_alive_reason_t;

typedef enum {
    U2F_HID_CAPABILITY_WINK = 0x01,
    U2F_HID_CAPABILITY_CBOR = 0x04,
    U2F_HID_CAPABILITY_NMSG = 0x08,
} u2f_hid_capability_t;

typedef enum {
    // FIDO 1
    CTAP1_ERR_SUCCESS           = 0x00,
    CTAP1_ERR_INVALID_COMMAND   = 0x01,
    CTAP1_ERR_INVALID_PARAMETER = 0x02,
    CTAP1_ERR_INVALID_LENGTH    = 0x03,
    CTAP1_ERR_INVALID_SEQ       = 0x04,
    CTAP1_ERR_TIMEOUT           = 0x05,
    CTAP1_ERR_CHANNEL_BUSY      = 0x06,  // HID only
    CTAP1_ERR_LOCK_REQUIRED     = 0x0A,  // HID only
    CTAP1_ERR_INVALID_CHANNEL   = 0x0B,  // HID only
    CTAP1_ERR_OTHER             = 0x7F,

    // FIDO2
    CTAP2_OK                          = 0x00,
    CTAP2_ERR_CBOR_UNEXPECTED_TYPE    = 0x11,
    CTAP2_ERR_INVALID_CBOR            = 0x12,
    CTAP2_ERR_MISSING_PARAMETER       = 0x14,
    CTAP2_ERR_LIMIT_EXCEEDED          = 0x15,
    CTAP2_ERR_FP_DATABASE_FULL        = 0x17,
    CTAP2_ERR_LARGE_BLOB_STORAGE_FULL = 0x18,
    CTAP2_ERR_CREDENTIAL_EXCLUDED     = 0x19,
    CTAP2_ERR_PROCESSING              = 0x21,
    CTAP2_ERR_INVALID_CREDENTIAL      = 0x22,
    CTAP2_ERR_USER_ACTION_PENDING     = 0x23,
    CTAP2_ERR_OPERATION_PENDING       = 0x24,
    CTAP2_ERR_NO_OPERATIONS           = 0x25,
    CTAP2_ERR_UNSUPPORTED_ALGORITHM   = 0x26,
    CTAP2_ERR_OPERATION_DENIED        = 0x27,
    CTAP2_ERR_KEY_STORE_FULL          = 0x28,
    CTAP2_ERR_UNSUPPORTED_OPTION      = 0x2B,
    CTAP2_ERR_INVALID_OPTION          = 0x2C,
    CTAP2_ERR_KEEPALIVE_CANCEL        = 0x2D,
    CTAP2_ERR_NO_CREDENTIALS          = 0x2E,
    CTAP2_ERR_USER_ACTION_TIMEOUT     = 0x2F,
    CTAP2_ERR_NOT_ALLOWED             = 0x30,
    CTAP2_ERR_PIN_INVALID             = 0x31,
    CTAP2_ERR_PIN_BLOCKED             = 0x32,
    CTAP2_ERR_PIN_AUTH_INVALID        = 0x33,
    CTAP2_ERR_PIN_AUTH_BLOCKED        = 0x34,
    CTAP2_ERR_PIN_NOT_SET             = 0x35,
    CTAP2_ERR_PUAT_REQUIRED           = 0x36,
    CTAP2_ERR_PIN_POLICY_VIOLATION    = 0x37,
    CTAP2_ERR_REQUEST_TOO_LARGE       = 0x39,
    CTAP2_ERR_ACTION_TIMEOUT          = 0x3A,
    CTAP2_ERR_UP_REQUIRED             = 0x3B,
    CTAP2_ERR_UV_BLOCKED              = 0x3C,
    CTAP2_ERR_INTEGRITY_FAILURE       = 0x3D,
    CTAP2_ERR_INVALID_SUBCOMMAND      = 0x3E,
    CTAP2_ERR_UV_INVALID              = 0x3F,
    CTAP2_ERR_UNAUTHORIZED_PERMISSION = 0x40,
    CTAP2_ERR_SPEC_LAST               = 0xDF,
    CTAP2_ERR_EXTENSION_FIRST         = 0xE0,
    CTAP2_ERR_EXTENSION_LAST          = 0xEF,
    CTAP2_ERR_VENDOR_FIRST            = 0xF0,
    CTAP2_ERR_VENDOR_LAST             = 0xFF,

    // Proprietary
    PROP_ERR_UNKNOWN_COMMAND          = 0x80,
    PROP_ERR_COMMAND_TOO_LONG         = 0x81,
    PROP_ERR_INVALID_CONTINUATION     = 0x82,
    PROP_ERR_UNEXPECTED_CONTINUATION  = 0x83,
    PROP_ERR_CONTINUATION_OVERFLOW    = 0x84,
    PROP_ERR_MESSAGE_TOO_SHORT        = 0x85,
    PROP_ERR_UNCONSISTENT_MSG_LENGTH  = 0x86,
    PROP_ERR_UNSUPPORTED_MSG_APDU     = 0x87,
    PROP_ERR_INVALID_DATA_LENGTH_APDU = 0x88,
    PROP_ERR_INTERNAL_ERROR_APDU      = 0x89,
    PROP_ERR_INVALID_PARAMETERS_APDU  = 0x8A,
    PROP_ERR_INVALID_DATA_APDU        = 0x8B,
    PROP_ERR_DEVICE_NOT_SETUP         = 0x8C,
    PROP_ERR_MEDIA_MIXED              = 0x8D,
    PROP_ERR_RPID_MEDIA_DENIED        = 0x8E,
} u2f_error_t;

/* Exported defines   --------------------------------------------------------*/
// LEGACY

// Shared commands
#define CTAP2_CMD_CBOR   U2F_COMMAND_HID_CBOR
#define CTAP2_CMD_CANCEL U2F_COMMAND_HID_CANCEL

// BLE only commands
#define KEEPALIVE_REASON_PROCESSING U2F_KEEP_ALIVE_REASON_PROCESSING
#define KEEPALIVE_REASON_TUP_NEEDED U2F_KEEP_ALIVE_REASON_UP_NEEDED

// Shared errors
#define ERROR_NONE                          CTAP1_ERR_SUCCESS
#define ERROR_INVALID_CMD                   CTAP1_ERR_INVALID_COMMAND
#define ERROR_INVALID_PAR                   CTAP1_ERR_INVALID_PARAMETER
#define ERROR_INVALID_LEN                   CTAP1_ERR_INVALID_LENGTH
#define ERROR_INVALID_SEQ                   CTAP1_ERR_INVALID_SEQ
#define ERROR_MSG_TIMEOUT                   CTAP1_ERR_TIMEOUT
#define ERROR_OTHER                         CTAP1_ERR_OTHER
// CTAP2 errors
#define ERROR_CBOR_UNEXPECTED_TYPE          CTAP2_ERR_CBOR_UNEXPECTED_TYPE
#define ERROR_INVALID_CBOR                  CTAP2_ERR_INVALID_CBOR
#define ERROR_MISSING_PARAMETER             CTAP2_ERR_MISSING_PARAMETER
#define ERROR_LIMIT_EXCEEDED                CTAP2_ERR_LIMIT_EXCEEDED
#define ERROR_CREDENTIAL_EXCLUDED           CTAP2_ERR_CREDENTIAL_EXCLUDED
#define ERROR_PROCESSING                    CTAP2_ERR_PROCESSING
#define ERROR_INVALID_CREDENTIAL            CTAP2_ERR_INVALID_CREDENTIAL
#define ERROR_USER_ACTION_PENDING           CTAP2_ERR_USER_ACTION_PENDING
#define ERROR_OPERATION_PENDING             CTAP2_ERR_OPERATION_PENDING
#define ERROR_NO_OPERATIONS                 CTAP2_ERR_NO_OPERATIONS
#define ERROR_UNSUPPORTED_ALGORITHM         CTAP2_ERR_UNSUPPORTED_ALGORITHM
#define ERROR_OPERATION_DENIED              CTAP2_ERR_OPERATION_DENIED
#define ERROR_KEY_STORE_FULL                CTAP2_ERR_KEY_STORE_FULL
#define ERROR_UNSUPPORTED_OPTION            CTAP2_ERR_UNSUPPORTED_OPTION
#define ERROR_INVALID_OPTION                CTAP2_ERR_INVALID_OPTION
#define ERROR_KEEPALIVE_CANCEL              CTAP2_ERR_KEEPALIVE_CANCEL
#define ERROR_NO_CREDENTIALS                CTAP2_ERR_NO_CREDENTIALS
#define ERROR_USER_ACTION_TIMEOUT           CTAP2_ERR_USER_ACTION_TIMEOUT
#define ERROR_NOT_ALLOWED                   CTAP2_ERR_NOT_ALLOWED
#define ERROR_PIN_INVALID                   CTAP2_ERR_PIN_INVALID
#define ERROR_PIN_BLOCKED                   CTAP2_ERR_PIN_BLOCKED
#define ERROR_PIN_AUTH_INVALID              CTAP2_ERR_PIN_AUTH_INVALID
#define ERROR_PIN_AUTH_BLOCKED              CTAP2_ERR_PIN_AUTH_BLOCKED
#define ERROR_PIN_NOT_SET                   CTAP2_ERR_PIN_NOT_SET
#define ERROR_PIN_REQUIRED                  CTAP2_ERR_PUAT_REQUIRED
#define ERROR_PIN_POLICY_VIOLATION          CTAP2_ERR_PIN_POLICY_VIOLATION
#define ERROR_REQUEST_TOO_LARGE             CTAP2_ERR_REQUEST_TOO_LARGE
#define ERROR_ACTION_TIMEOUT                CTAP2_ERR_ACTION_TIMEOUT
#define ERROR_UP_REQUIRED                   CTAP2_ERR_UP_REQUIRED
// Proprietary errors
#define ERROR_PROP_UNKNOWN_COMMAND          PROP_ERR_UNKNOWN_COMMAND
#define ERROR_PROP_COMMAND_TOO_LONG         PROP_ERR_COMMAND_TOO_LONG
#define ERROR_PROP_INVALID_CONTINUATION     PROP_ERR_INVALID_CONTINUATION
#define ERROR_PROP_UNEXPECTED_CONTINUATION  PROP_ERR_UNEXPECTED_CONTINUATION
#define ERROR_PROP_CONTINUATION_OVERFLOW    PROP_ERR_CONTINUATION_OVERFLOW
#define ERROR_PROP_MESSAGE_TOO_SHORT        PROP_ERR_MESSAGE_TOO_SHORT
#define ERROR_PROP_UNCONSISTENT_MSG_LENGTH  PROP_ERR_UNCONSISTENT_MSG_LENGTH
#define ERROR_PROP_UNSUPPORTED_MSG_APDU     PROP_ERR_UNSUPPORTED_MSG_APDU
#define ERROR_PROP_INVALID_DATA_LENGTH_APDU PROP_ERR_INVALID_DATA_LENGTH_APDU
#define ERROR_PROP_INTERNAL_ERROR_APDU      PROP_ERR_INTERNAL_ERROR_APDU
#define ERROR_PROP_INVALID_PARAMETERS_APDU  PROP_ERR_INVALID_PARAMETERS_APDU
#define ERROR_PROP_INVALID_DATA_APDU        PROP_ERR_INVALID_DATA_APDU
#define ERROR_PROP_DEVICE_NOT_SETUP         PROP_ERR_DEVICE_NOT_SETUP
#define ERROR_PROP_MEDIA_MIXED              PROP_ERR_MEDIA_MIXED
#define ERROR_PROP_RPID_MEDIA_DENIED        PROP_ERR_RPID_MEDIA_DENIED

#define U2F_CMD_MSG U2F_COMMAND_MSG

/* Exported macros------------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/

/* Exported functions prototypes--------------------------------------------- */
