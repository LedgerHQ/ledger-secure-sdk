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

/**
 * @file  ble_ledger_profile_u2f.c
 * @brief FIDO BLE GATT profile implementing CTAP2 §8.3 BLE Transport Binding.
 *
 * This profile registers the FIDO GATT service (UUID 0xFFFD) with four
 * characteristics mandated by the spec:
 *   - fidoControlPoint       (Write)            – receives CTAP2-BLE frames
 *   - fidoStatus             (Notify)           – sends CTAP2-BLE frames back
 *   - fidoControlPointLength (Read)             – advertises max write size
 *   - fidoServiceRevisionBitfield (Read/Write)  – supported FIDO versions
 *
 * Framing follows the CTAP2 BLE specification: initialisation packets carry
 * CMD(1)|HLEN(1)|LLEN(1)|DATA, continuation packets carry SEQ(1)|DATA.
 * The SDK's lib_u2f U2F_TRANSPORT layer handles the reassembly/fragmentation
 * when configured with U2F_TRANSPORT_TYPE_BLE.
 */

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <string.h>
#include "os.h"
#include "os_pic.h"
#include "os_io.h"
#include "ble_ledger.h"
#include "ble_ledger_profile_u2f.h"
#include "u2f_transport.h"

#ifdef HAVE_PRINTF
#define LOG_IO PRINTF
#else
#define LOG_IO(...)
#endif

/* Private enumerations ------------------------------------------------------*/
typedef enum {
    U2F_PROFILE_STATE_IDLE,
    U2F_PROFILE_STATE_BUSY,
} u2f_profile_state_t;

typedef enum {
    U2F_CREATE_DB_STEP_IDLE,
    U2F_CREATE_DB_STEP_ADD_SERVICE,
    U2F_CREATE_DB_STEP_ADD_CONTROL_POINT_CHAR,
    U2F_CREATE_DB_STEP_ADD_STATUS_CHAR,
    U2F_CREATE_DB_STEP_ADD_CONTROL_POINT_LENGTH_CHAR,
    U2F_CREATE_DB_STEP_ADD_SERVICE_REVISION_CHAR,
    U2F_CREATE_DB_STEP_SET_CONTROL_POINT_LENGTH_VALUE,
    U2F_CREATE_DB_STEP_SET_SERVICE_REVISION_VALUE,
    U2F_CREATE_DB_STEP_END,
} u2f_create_db_step_t;

/* Private types, structures, unions -----------------------------------------*/
typedef struct {
    /* DB creation state machine */
    u2f_create_db_step_t create_db_step;

    /* Profile state */
    u2f_profile_state_t state;

    /* U2F Transport (handles BLE framing per CTAP2 §8.3) */
    u2f_transport_t transport;

    /* Encryption */
    bool link_is_encrypted;

    /* Connection */
    ble_connection_t *connection;

    /* ATT/GATT handles */
    uint16_t gatt_service_handle;
    uint16_t gatt_control_point_char_handle;        /* fidoControlPoint (Write) */
    uint16_t gatt_status_char_handle;               /* fidoStatus (Notify) */
    uint16_t gatt_control_point_length_char_handle; /* fidoControlPointLength (Read) */
    uint16_t gatt_service_revision_char_handle;     /* fidoServiceRevisionBitfield (Read/Write) */

    /* MTU tracking – effective max fragment size (ATT MTU - 3 for notify) */
    uint16_t mtu;
    uint8_t  mtu_negotiated;
    uint8_t  notifications_enabled;
    uint8_t  wait_write_resp_ack;

    /* BLE CMD data pointer (shared with ble_ledger.c) */
    ble_cmd_data_t *cmd_data;

} ledger_ble_profile_u2f_handle_t;

/* Private macros-------------------------------------------------------------*/
#define FIDO_BLE_DEFAULT_MTU  (BLE_ATT_DEFAULT_MTU - 3)

/* Private functions prototypes ----------------------------------------------*/
static void notify_fragment(ledger_ble_profile_u2f_handle_t *handle);

/* Private variables ---------------------------------------------------------*/
static ledger_ble_profile_u2f_handle_t ledger_u2f_profile_handle;

/**
 * TX packet buffer used by U2F_TRANSPORT_tx for building outgoing BLE
 * fragments. Sized to the maximum ATT MTU so we can send one full
 * notification payload.
 */
static uint8_t u2f_tx_packet_buffer[BLE_ATT_MAX_MTU_SIZE];

/**
 * RX message buffer – reassembled from BLE fragments.
 * Sized to OS_IO_BUFFER_SIZE to hold a full CTAP2 command.
 */
static uint8_t u2f_rx_message_buffer[OS_IO_BUFFER_SIZE + 1];

/* FIDO BLE characteristic UUIDs (128-bit, little-endian for BLE stack) */
static const uint8_t fidoControlPointUuid[BLE_UUID_SIZE]
    = BLE_FIDO_CHAR_UUID_CONTROL_POINT;
static const uint8_t fidoStatusUuid[BLE_UUID_SIZE]
    = BLE_FIDO_CHAR_UUID_STATUS;
static const uint8_t fidoControlPointLengthUuid[BLE_UUID_SIZE]
    = BLE_FIDO_CHAR_UUID_CONTROL_POINT_LENGTH;
static const uint8_t fidoServiceRevisionBitfieldUuid[BLE_UUID_SIZE]
    = BLE_FIDO_CHAR_UUID_SERVICE_REVISION_BITFIELD;

/* FIDO service UUID as 16-bit LE for the GATT add_service call */
static const uint8_t fidoServiceUuid16[2] = {
    (BLE_FIDO_SERVICE_UUID_16 & 0xFF),
    (BLE_FIDO_SERVICE_UUID_16 >> 8) & 0xFF,
};

/* Exported variables --------------------------------------------------------*/
const ble_profile_info_t BLE_LEDGER_PROFILE_u2f_info = {
    .type = BLE_LEDGER_PROFILE_U2F,

    /* 16-bit FIDO UUID 0xFFFD, stored in the same ble_uuid_t used for scan rsp */
    .service_uuid = {
        .type  = BLE_GATT_UUID_TYPE_16,
        .value = {(BLE_FIDO_SERVICE_UUID_16 & 0xFF),
                  (BLE_FIDO_SERVICE_UUID_16 >> 8) & 0xFF},
    },

    .init            = BLE_LEDGER_PROFILE_u2f_init,
    .create_db       = BLE_LEDGER_PROFILE_u2f_create_db,
    .handle_in_range = BLE_LEDGER_PROFILE_u2f_handle_in_range,

    .connection_evt        = BLE_LEDGER_PROFILE_u2f_connection_evt,
    .connection_update_evt = BLE_LEDGER_PROFILE_u2f_connection_update_evt,
    .encryption_changed    = BLE_LEDGER_PROFILE_u2f_encryption_changed,

    .att_modified     = BLE_LEDGER_PROFILE_u2f_att_modified,
    .write_permit_req = BLE_LEDGER_PROFILE_u2f_write_permit_req,
    .mtu_changed      = BLE_LEDGER_PROFILE_u2f_mtu_changed,

    .write_rsp_ack       = BLE_LEDGER_PROFILE_u2f_write_rsp_ack,
    .update_char_val_ack = BLE_LEDGER_PROFILE_u2f_update_char_value_ack,

    .send_packet = BLE_LEDGER_PROFILE_u2f_send_packet,
    .is_busy     = BLE_LEDGER_PROFILE_u2f_is_busy,

    .data_ready = BLE_LEDGER_PROFILE_u2f_data_ready,

    .setting = BLE_LEDGER_PROFILE_u2f_setting,

    .cookie = &ledger_u2f_profile_handle,
};

/* Private functions ---------------------------------------------------------*/

/**
 * Send the current TX fragment via a GATT notification on fidoStatus.
 */
static void notify_fragment(ledger_ble_profile_u2f_handle_t *handle)
{
    if (handle->transport.tx_packet_length > 0) {
        ble_aci_gatt_forge_cmd_update_char_value(
            handle->cmd_data,
            handle->gatt_service_handle,
            handle->gatt_status_char_handle,
            0,
            handle->transport.tx_packet_length,
            u2f_tx_packet_buffer);
    }
}

/* Exported functions --------------------------------------------------------*/

ble_profile_status_t BLE_LEDGER_PROFILE_u2f_init(ble_cmd_data_t *cmd_data,
                                                  void           *cookie)
{
    ble_profile_status_t status = BLE_PROFILE_STATUS_BAD_PARAMETERS;
    if (!cmd_data || !cookie) {
        goto error;
    }

    ledger_ble_profile_u2f_handle_t *handle
        = (ledger_ble_profile_u2f_handle_t *) PIC(cookie);

    memset(handle, 0, sizeof(ledger_ble_profile_u2f_handle_t));
    handle->cmd_data = cmd_data;
    handle->mtu      = FIDO_BLE_DEFAULT_MTU;

    /* Initialise the U2F transport layer in BLE mode */
    U2F_TRANSPORT_init(&handle->transport, U2F_TRANSPORT_TYPE_BLE);
    handle->transport.rx_message_buffer      = u2f_rx_message_buffer;
    handle->transport.rx_message_buffer_size = sizeof(u2f_rx_message_buffer);

    handle->gatt_service_handle                     = BLE_GATT_INVALID_HANDLE;
    handle->gatt_control_point_char_handle          = BLE_GATT_INVALID_HANDLE;
    handle->gatt_status_char_handle                 = BLE_GATT_INVALID_HANDLE;
    handle->gatt_control_point_length_char_handle   = BLE_GATT_INVALID_HANDLE;
    handle->gatt_service_revision_char_handle       = BLE_GATT_INVALID_HANDLE;
    handle->create_db_step                          = U2F_CREATE_DB_STEP_IDLE;

    status = BLE_PROFILE_STATUS_OK;

error:
    return status;
}

ble_profile_status_t BLE_LEDGER_PROFILE_u2f_create_db(uint8_t  *hci_buffer,
                                                       uint16_t  length,
                                                       void     *cookie)
{
    ble_profile_status_t status = BLE_PROFILE_STATUS_BAD_PARAMETERS;
    if (!hci_buffer || !cookie) {
        goto error;
    }

    ledger_ble_profile_u2f_handle_t *handle
        = (ledger_ble_profile_u2f_handle_t *) PIC(cookie);

    /* Parse handle returned from previous step */
    if (handle->create_db_step == U2F_CREATE_DB_STEP_IDLE) {
        LOG_IO("U2F PROFILE INIT START\n");
    }
    else if ((length >= 2)
             && (handle->create_db_step == U2F_CREATE_DB_STEP_ADD_SERVICE)) {
        handle->gatt_service_handle = U2LE(hci_buffer, 4);
        LOG_IO("U2F FIDO service handle              : %04X\n",
               handle->gatt_service_handle);
    }
    else if ((length >= 2)
             && (handle->create_db_step == U2F_CREATE_DB_STEP_ADD_CONTROL_POINT_CHAR)) {
        handle->gatt_control_point_char_handle = U2LE(hci_buffer, 4);
        LOG_IO("U2F fidoControlPoint char handle     : %04X\n",
               handle->gatt_control_point_char_handle);
    }
    else if ((length >= 2)
             && (handle->create_db_step == U2F_CREATE_DB_STEP_ADD_STATUS_CHAR)) {
        handle->gatt_status_char_handle = U2LE(hci_buffer, 4);
        LOG_IO("U2F fidoStatus char handle           : %04X\n",
               handle->gatt_status_char_handle);
    }
    else if ((length >= 2)
             && (handle->create_db_step
                 == U2F_CREATE_DB_STEP_ADD_CONTROL_POINT_LENGTH_CHAR)) {
        handle->gatt_control_point_length_char_handle = U2LE(hci_buffer, 4);
        LOG_IO("U2F fidoControlPointLength handle    : %04X\n",
               handle->gatt_control_point_length_char_handle);
    }
    else if ((length >= 2)
             && (handle->create_db_step
                 == U2F_CREATE_DB_STEP_ADD_SERVICE_REVISION_CHAR)) {
        handle->gatt_service_revision_char_handle = U2LE(hci_buffer, 4);
        LOG_IO("U2F fidoServiceRevisionBitfield handle: %04X\n",
               handle->gatt_service_revision_char_handle);
    }
    /* Steps SET_CONTROL_POINT_LENGTH_VALUE and SET_SERVICE_REVISION_VALUE
       have no handle to parse — just acknowledge the response. */

    handle->create_db_step++;

    switch (handle->create_db_step) {
        case U2F_CREATE_DB_STEP_ADD_SERVICE:
            /* FIDO service uses 16-bit UUID 0xFFFD */
            ble_aci_gatt_forge_cmd_add_service(
                handle->cmd_data,
                BLE_GATT_UUID_TYPE_16,
                fidoServiceUuid16,
                BLE_GATT_PRIMARY_SERVICE,
                BLE_FIDO_MAX_SERVICE_RECORDS);
            status = BLE_PROFILE_STATUS_OK;
            break;

        case U2F_CREATE_DB_STEP_ADD_CONTROL_POINT_CHAR: {
            /* fidoControlPoint – Write (host → authenticator) */
            ble_cmd_add_char_data_t data;
            data.service_handle       = handle->gatt_service_handle;
            data.char_uuid_type       = BLE_GATT_UUID_TYPE_128;
            data.char_uuid            = fidoControlPointUuid;
            data.char_value_length    = BLE_ATT_MAX_MTU_SIZE;
            data.char_properties      = BLE_CHAR_PROP_WRITE;
            data.security_permissions = BLE_GATT_ATTR_PERMISSION_AUTHEN_WRITE;
            data.gatt_evt_mask        = BLE_GATT_NOTIFY_WRITE_REQ_AND_WAIT_FOR_APPL_RESP;
            data.enc_key_size         = BLE_GAP_MAX_ENCRYPTION_KEY_SIZE;
            data.is_variable          = 1;
            ble_aci_gatt_forge_cmd_add_char(handle->cmd_data, &data);
            status = BLE_PROFILE_STATUS_OK;
            break;
        }

        case U2F_CREATE_DB_STEP_ADD_STATUS_CHAR: {
            /* fidoStatus – Notify (authenticator → host) */
            ble_cmd_add_char_data_t data;
            data.service_handle       = handle->gatt_service_handle;
            data.char_uuid_type       = BLE_GATT_UUID_TYPE_128;
            data.char_uuid            = fidoStatusUuid;
            data.char_value_length    = BLE_ATT_MAX_MTU_SIZE;
            data.char_properties      = BLE_CHAR_PROP_NOTIFY;
            data.security_permissions = BLE_GATT_ATTR_PERMISSION_AUTHEN_WRITE;
            data.gatt_evt_mask        = BLE_GATT_DONT_NOTIFY_EVENTS;
            data.enc_key_size         = BLE_GAP_MAX_ENCRYPTION_KEY_SIZE;
            data.is_variable          = 1;
            ble_aci_gatt_forge_cmd_add_char(handle->cmd_data, &data);
            status = BLE_PROFILE_STATUS_OK;
            break;
        }

        case U2F_CREATE_DB_STEP_ADD_CONTROL_POINT_LENGTH_CHAR: {
            /* fidoControlPointLength – Read (2 bytes, big-endian max write size) */
            ble_cmd_add_char_data_t data;
            data.service_handle       = handle->gatt_service_handle;
            data.char_uuid_type       = BLE_GATT_UUID_TYPE_128;
            data.char_uuid            = fidoControlPointLengthUuid;
            data.char_value_length    = 2;
            data.char_properties      = BLE_CHAR_PROP_READ;
            data.security_permissions = 0;
            data.gatt_evt_mask        = BLE_GATT_DONT_NOTIFY_EVENTS;
            data.enc_key_size         = BLE_GAP_MAX_ENCRYPTION_KEY_SIZE;
            data.is_variable          = 0;
            ble_aci_gatt_forge_cmd_add_char(handle->cmd_data, &data);
            status = BLE_PROFILE_STATUS_OK;
            break;
        }

        case U2F_CREATE_DB_STEP_ADD_SERVICE_REVISION_CHAR: {
            /* fidoServiceRevisionBitfield – Read | Write (1 byte) */
            ble_cmd_add_char_data_t data;
            data.service_handle       = handle->gatt_service_handle;
            data.char_uuid_type       = BLE_GATT_UUID_TYPE_128;
            data.char_uuid            = fidoServiceRevisionBitfieldUuid;
            data.char_value_length    = 1;
            data.char_properties      = BLE_CHAR_PROP_READ | BLE_CHAR_PROP_WRITE;
            data.security_permissions = 0;
            data.gatt_evt_mask        = BLE_GATT_NOTIFY_ATTRIBUTE_WRITE;
            data.enc_key_size         = BLE_GAP_MAX_ENCRYPTION_KEY_SIZE;
            data.is_variable          = 0;
            ble_aci_gatt_forge_cmd_add_char(handle->cmd_data, &data);
            status = BLE_PROFILE_STATUS_OK;
            break;
        }

        case U2F_CREATE_DB_STEP_SET_CONTROL_POINT_LENGTH_VALUE: {
            /* Set initial value of fidoControlPointLength (big-endian) */
            uint8_t cp_length_val[2];
            U2BE_ENCODE(cp_length_val, 0, BLE_ATT_MAX_MTU_SIZE);
            ble_aci_gatt_forge_cmd_update_char_value(
                handle->cmd_data,
                handle->gatt_service_handle,
                handle->gatt_control_point_length_char_handle,
                0,
                2,
                cp_length_val);
            status = BLE_PROFILE_STATUS_OK;
            break;
        }

        case U2F_CREATE_DB_STEP_SET_SERVICE_REVISION_VALUE: {
            /* Set initial value of fidoServiceRevisionBitfield */
            uint8_t rev_val = FIDO_BLE_SERVICE_REVISION_BITFIELD;
            ble_aci_gatt_forge_cmd_update_char_value(
                handle->cmd_data,
                handle->gatt_service_handle,
                handle->gatt_service_revision_char_handle,
                0,
                1,
                &rev_val);
            status = BLE_PROFILE_STATUS_OK;
            break;
        }

        case U2F_CREATE_DB_STEP_END:
            LOG_IO("U2F PROFILE INIT END\n");
            status = BLE_PROFILE_STATUS_OK_PROCEDURE_END;
            break;

        default:
            break;
    }

error:
    return status;
}

uint8_t BLE_LEDGER_PROFILE_u2f_handle_in_range(uint16_t gatt_handle, void *cookie)
{
    if (!cookie) {
        return 0;
    }

    ledger_ble_profile_u2f_handle_t *handle
        = (ledger_ble_profile_u2f_handle_t *) PIC(cookie);

    if ((gatt_handle >= handle->gatt_service_handle)
        && (gatt_handle <= handle->gatt_service_revision_char_handle + 2)) {
        return 1;
    }

    return 0;
}

void BLE_LEDGER_PROFILE_u2f_connection_evt(ble_connection_t *connection,
                                            void             *cookie)
{
    if (!cookie || !connection) {
        return;
    }

    ledger_ble_profile_u2f_handle_t *handle
        = (ledger_ble_profile_u2f_handle_t *) PIC(cookie);

    handle->notifications_enabled = 0;
    handle->mtu                   = FIDO_BLE_DEFAULT_MTU;
    handle->mtu_negotiated        = 0;
    handle->wait_write_resp_ack   = 0;
    handle->connection            = connection;
    handle->state                 = U2F_PROFILE_STATE_IDLE;

    /* Re-initialise BLE transport for the new connection */
    U2F_TRANSPORT_init(&handle->transport, U2F_TRANSPORT_TYPE_BLE);
    handle->transport.rx_message_buffer      = u2f_rx_message_buffer;
    handle->transport.rx_message_buffer_size = sizeof(u2f_rx_message_buffer);
}

void BLE_LEDGER_PROFILE_u2f_connection_update_evt(ble_connection_t *connection,
                                                   void             *cookie)
{
    if (!cookie || !connection) {
        return;
    }

    ledger_ble_profile_u2f_handle_t *handle
        = (ledger_ble_profile_u2f_handle_t *) PIC(cookie);

    handle->connection = connection;
}

void BLE_LEDGER_PROFILE_u2f_encryption_changed(bool encrypted, void *cookie)
{
    if (!cookie) {
        return;
    }

    ledger_ble_profile_u2f_handle_t *handle
        = (ledger_ble_profile_u2f_handle_t *) PIC(cookie);

    handle->link_is_encrypted = encrypted;
}

ble_profile_status_t BLE_LEDGER_PROFILE_u2f_att_modified(uint8_t  *hci_buffer,
                                                          uint16_t  length,
                                                          void     *cookie)
{
    if (!hci_buffer || !cookie || (length < 10)) {
        return BLE_PROFILE_STATUS_BAD_PARAMETERS;
    }

    ble_profile_status_t status = BLE_PROFILE_STATUS_OK;
    ledger_ble_profile_u2f_handle_t *handle
        = (ledger_ble_profile_u2f_handle_t *) PIC(cookie);

    uint16_t connection_handle = U2LE(hci_buffer, 2);
    uint16_t att_handle        = U2LE(hci_buffer, 4);
    uint16_t offset            = U2LE(hci_buffer, 6);
    uint16_t att_data_length   = U2LE(hci_buffer, 8);
    UNUSED(connection_handle);

    /* Check if this is the fidoStatus CCCD (Notify enable/disable) */
    if ((att_handle == handle->gatt_status_char_handle + 2)
        && (att_data_length == 2) && (offset == 0)) {
        if (U2LE(hci_buffer, 10) != 0) {
            LOG_IO("U2F REGISTERED FOR NOTIFICATIONS\n");
            handle->notifications_enabled = 1;
            if (!handle->mtu_negotiated) {
                ble_aci_gatt_forge_cmd_exchange_config(
                    handle->cmd_data, connection_handle);
                status = BLE_PROFILE_STATUS_OK_AND_SEND_PACKET;
            }
        } else {
            LOG_IO("U2F NOT REGISTERED FOR NOTIFICATIONS\n");
            handle->notifications_enabled = 0;
        }
    }
    /* Check if this is a write to fidoServiceRevisionBitfield */
    else if ((att_handle == handle->gatt_service_revision_char_handle + 1)
             && (att_data_length == 1)) {
        uint8_t requested = hci_buffer[10];
        UNUSED(requested);
        LOG_IO("U2F SERVICE REVISION WRITE: 0x%02X\n", requested);
        /* Accept only bits we support */
        /* No action needed – the stack stores the value */
    }

    return status;
}

ble_profile_status_t BLE_LEDGER_PROFILE_u2f_write_permit_req(uint8_t  *hci_buffer,
                                                              uint16_t  length,
                                                              void     *cookie)
{
    ble_profile_status_t status = BLE_PROFILE_STATUS_BAD_PARAMETERS;
    if (!hci_buffer || !cookie || (length < 7)) {
        goto error;
    }

    ledger_ble_profile_u2f_handle_t *handle
        = (ledger_ble_profile_u2f_handle_t *) PIC(cookie);

    uint16_t connection_handle = U2LE(hci_buffer, 2);
    uint16_t att_handle        = U2LE(hci_buffer, 4);
    uint8_t  data_length       = hci_buffer[6];

    if (data_length > length - 7) {
        goto error;
    }

    handle->wait_write_resp_ack = 1;

    /* Write to fidoControlPoint – a CTAP2 BLE frame fragment */
    if ((att_handle == handle->gatt_control_point_char_handle + 1)
        && (handle->notifications_enabled)
        && (handle->link_is_encrypted)
        && (data_length > 0)) {

        LOG_IO("U2F CONTROL POINT WRITE %d bytes\n", data_length);

        /* Feed the raw BLE fragment into the U2F transport layer.
         * The transport layer handles init/continuation reassembly. */
        U2F_TRANSPORT_rx(&handle->transport, &hci_buffer[7], data_length);

        /* Always ACK the write */
        ble_aci_gatt_forge_cmd_write_resp(handle->cmd_data,
                                          connection_handle,
                                          att_handle,
                                          0,
                                          HCI_SUCCESS_ERR_CODE,
                                          data_length,
                                          &hci_buffer[7]);

        /* If transport flagged an error, send an ERROR response */
        if (handle->transport.error != CTAP1_ERR_SUCCESS) {
            LOG_IO("U2F TRANSPORT ERROR: 0x%02X\n", handle->transport.error);
            uint8_t err_byte = (uint8_t) handle->transport.error;
            U2F_TRANSPORT_tx(&handle->transport,
                             U2F_COMMAND_ERROR & 0x7F,
                             &err_byte,
                             1,
                             u2f_tx_packet_buffer,
                             handle->mtu);
            handle->transport.state = U2F_STATE_IDLE;
        }
    } else {
        LOG_IO("U2F ATT WRITE %04X %d bytes (ignored)\n", att_handle, data_length);
        ble_aci_gatt_forge_cmd_write_resp(handle->cmd_data,
                                          connection_handle,
                                          att_handle,
                                          0,
                                          HCI_SUCCESS_ERR_CODE,
                                          data_length,
                                          &hci_buffer[7]);
    }
    status = BLE_PROFILE_STATUS_OK_AND_SEND_PACKET;

error:
    return status;
}

ble_profile_status_t BLE_LEDGER_PROFILE_u2f_mtu_changed(uint16_t mtu, void *cookie)
{
    ble_profile_status_t status = BLE_PROFILE_STATUS_BAD_PARAMETERS;
    if (!cookie) {
        goto error;
    }

    if ((mtu > 3) && (mtu <= BLE_ATT_MAX_MTU_SIZE)) {
        ledger_ble_profile_u2f_handle_t *handle
            = (ledger_ble_profile_u2f_handle_t *) PIC(cookie);
        /* For notifications, usable payload = ATT_MTU - 3 */
        handle->mtu            = mtu - 3;
        handle->mtu_negotiated = 1;

        /* Update fidoControlPointLength to reflect the new max write size */
        uint8_t cp_length_val[2];
        U2BE_ENCODE(cp_length_val, 0, mtu - 3);
        ble_aci_gatt_forge_cmd_update_char_value(
            handle->cmd_data,
            handle->gatt_service_handle,
            handle->gatt_control_point_length_char_handle,
            0,
            2,
            cp_length_val);

        status = BLE_PROFILE_STATUS_OK;
    }

error:
    return status;
}

ble_profile_status_t BLE_LEDGER_PROFILE_u2f_write_rsp_ack(void *cookie)
{
    ble_profile_status_t status = BLE_PROFILE_STATUS_OK;
    if (!cookie) {
        return BLE_PROFILE_STATUS_BAD_PARAMETERS;
    }

    ledger_ble_profile_u2f_handle_t *handle
        = (ledger_ble_profile_u2f_handle_t *) PIC(cookie);

    if (handle->wait_write_resp_ack != 0) {
        handle->wait_write_resp_ack = 0;
        /* If there is a TX fragment ready to send, push it now */
        if (handle->transport.tx_packet_length > 0) {
            notify_fragment(handle);
            status = BLE_PROFILE_STATUS_OK_AND_SEND_PACKET;
        }
    }

    return status;
}

ble_profile_status_t BLE_LEDGER_PROFILE_u2f_update_char_value_ack(void *cookie)
{
    ble_profile_status_t status = BLE_PROFILE_STATUS_OK;
    if (!cookie) {
        return BLE_PROFILE_STATUS_BAD_PARAMETERS;
    }

    ledger_ble_profile_u2f_handle_t *handle
        = (ledger_ble_profile_u2f_handle_t *) PIC(cookie);

    handle->transport.tx_packet_length = 0;

    /* Check if there are more fragments to send (continuation packets) */
    if (handle->transport.tx_message_buffer != NULL) {
        U2F_TRANSPORT_tx(&handle->transport,
                         0,    /* cmd ignored for continuation */
                         NULL, /* NULL = continuation packet */
                         0,
                         u2f_tx_packet_buffer,
                         handle->mtu);

        if (handle->transport.tx_packet_length > 0) {
            notify_fragment(handle);
            status = BLE_PROFILE_STATUS_OK_AND_SEND_PACKET;
        }
    } else {
        /* All fragments sent – mark profile idle */
        handle->state = U2F_PROFILE_STATE_IDLE;
    }

    return status;
}

bool BLE_LEDGER_PROFILE_u2f_is_busy(void *cookie)
{
    ledger_ble_profile_u2f_handle_t *handle
        = (ledger_ble_profile_u2f_handle_t *) PIC(cookie);

    return (handle->state == U2F_PROFILE_STATE_BUSY);
}

ble_profile_status_t BLE_LEDGER_PROFILE_u2f_send_packet(const uint8_t *packet,
                                                         uint16_t       length,
                                                         void          *cookie)
{
    ble_profile_status_t status = BLE_PROFILE_STATUS_BAD_PARAMETERS;
    if (!packet || !cookie) {
        goto error;
    }

    ledger_ble_profile_u2f_handle_t *handle
        = (ledger_ble_profile_u2f_handle_t *) PIC(cookie);

    /*
     * The packet coming from the application contains:
     *   packet[0] = CTAP2-BLE command byte (without 0x80 bit)
     *   packet[1..length-1] = payload
     *
     * We need to determine the correct FIDO BLE command byte.
     * The os_io_tx_cmd path passes the raw U2F command as byte 0.
     */
    uint8_t cmd = packet[0];

    U2F_TRANSPORT_tx(&handle->transport,
                     cmd,
                     &packet[1],
                     length - 1,
                     u2f_tx_packet_buffer,
                     handle->mtu);

    if (handle->transport.tx_packet_length > 0) {
        handle->state = U2F_PROFILE_STATE_BUSY;
        if (handle->wait_write_resp_ack == 0) {
            notify_fragment(handle);
            status = BLE_PROFILE_STATUS_OK_AND_SEND_PACKET;
        } else {
            status = BLE_PROFILE_STATUS_OK;
        }
    } else {
        status = BLE_PROFILE_STATUS_OK;
    }

error:
    return status;
}

int32_t BLE_LEDGER_PROFILE_u2f_data_ready(uint8_t  *buffer,
                                           uint16_t  max_length,
                                           void     *cookie)
{
    int32_t result = 0;

    if (!buffer || !cookie) {
        return -1;
    }

    ledger_ble_profile_u2f_handle_t *handle
        = (ledger_ble_profile_u2f_handle_t *) PIC(cookie);

    if (handle->transport.state == U2F_STATE_CMD_COMPLETE) {
        uint16_t msg_len = handle->transport.rx_message_length;

        /* Need 1 extra byte for the packet type prefix */
        if ((msg_len == 0) || ((msg_len + 1) > max_length)
            || (msg_len > sizeof(u2f_rx_message_buffer))) {
            result = -1;
        } else {
            /* Prepend OS_IO_PACKET_TYPE_BLE_U2F_APDU (0x31) so that the IO
             * dispatch layer knows this came from BLE FIDO transport.
             * This mirrors what LEDGER_PROTOCOL does for BLE APDU. */
            buffer[0] = OS_IO_PACKET_TYPE_BLE_U2F_APDU;
            memmove(&buffer[1], u2f_rx_message_buffer, msg_len);
            result = msg_len + 1;  /* include the type byte in length */
        }

        /* Mark as consumed */
        handle->transport.state              = U2F_STATE_CMD_PROCESSING;
        handle->transport.rx_message_length  = 0;
        handle->transport.rx_message_offset  = 0;
    }

    return result;
}

void BLE_LEDGER_PROFILE_u2f_setting(uint32_t  id,
                                     uint8_t  *buffer,
                                     uint16_t  length,
                                     void     *cookie)
{
    /* No profile-specific settings for FIDO BLE at this time */
    UNUSED(id);
    UNUSED(buffer);
    UNUSED(length);
    UNUSED(cookie);
}
