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

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <string.h>
#include "os.h"
#include "os_pic.h"
#include "errors.h"
#include "ledger_protocol.h"
#include "ble_ledger.h"
#include "ble_ledger_profile_apdu.h"

#ifdef HAVE_PRINTF
#define LOG_IO PRINTF
// #define DELOG_IOBUG(...)
#else  // !HAVE_PRINTF
#define LOG_IO(...)
#endif  // !HAVE_PRINTF

/* Private enumerations ------------------------------------------------------*/
typedef enum ledger_ble_profile_apdu_state_e {
    LEDGER_BLE_PROFILE_APDU_STATE_IDLE,
    LEDGER_BLE_PROFILE_APDU_BUSY,
} ledger_ble_profile_apdu_state_t;

typedef enum create_db_step_e {
    CREATE_DB_STEP_IDLE,
    CREATE_DB_STEP_ADD_SERVICE,
    CREATE_DB_STEP_ADD_NOTIFICATION_CHARACTERISTIC,
    CREATE_DB_STEP_ADD_WRITE_CHARACTERISTIC,
    CREATE_DB_STEP_ADD_WRITE_COMMAND_CHARACTERISTIC,
    CREATE_DB_STEP_END,
} create_db_step_t;

/* Private defines------------------------------------------------------------*/
#define CONN_INTERVAL_MIN 12  // 15ms

/* Private types, structures, unions -----------------------------------------*/
typedef struct ledger_ble_profile_apdu_handle_s {
    // Database creation state
    create_db_step_t create_db_step;

    ledger_ble_profile_apdu_state_t state;

    // LEDGER PROTOCOL
    ledger_protocol_t protocol_data;

    // ENCRYPTION
    bool link_is_encrypted;

    // CONNECTION
    ble_connection_t *connection;
    uint8_t           connection_updated;

    // ATT/GATT
    uint16_t gatt_service_handle;
    uint16_t gatt_notification_characteristic_handle;
    uint16_t gatt_write_characteristic_handle;
    uint16_t gatt_write_cmd_characteristic_handle;
    uint8_t  mtu_negotiated;
    uint8_t  notifications_enabled;
    uint8_t  wait_write_resp_ack;

    // BLE CMD
    ble_cmd_data_t *cmd_data;

    // TRANSFER MODE
    uint8_t transfer_mode_wanted_enabled;
    uint8_t transfer_mode_enabled;
    bool send_response;
    uint8_t resp[2];

} ledger_ble_profile_apdu_handle_t;

/* Private macros-------------------------------------------------------------*/

/* Private functions prototypes ----------------------------------------------*/
static void notify_chunk(ledger_ble_profile_apdu_handle_t *handle);
static void check_transfer_mode(ledger_ble_profile_apdu_handle_t *handle);

/* Private variables ---------------------------------------------------------*/
static ledger_ble_profile_apdu_handle_t ledger_apdu_profile_handle;
static uint8_t                          ble_ledger_protocol_chunk_buffer[BLE_ATT_MAX_MTU_SIZE];

// clang-format off
#ifdef TARGET_FLEX
const uint8_t charUuidTX[BLE_UUID_SIZE]   = {0x72,0x65,0x67,0x64,0x65,0x4c,0x01,0x00,0x04,0x30,0x97,0x2c,0x00,0x34,0xd6,0x13};
const uint8_t charUuidRX[BLE_UUID_SIZE]   = {0x72,0x65,0x67,0x64,0x65,0x4c,0x02,0x00,0x04,0x30,0x97,0x2c,0x00,0x34,0xd6,0x13};
const uint8_t charUuidRX2[BLE_UUID_SIZE]  = {0x72,0x65,0x67,0x64,0x65,0x4c,0x03,0x00,0x04,0x30,0x97,0x2c,0x00,0x34,0xd6,0x13};
#else // !TARGET_FLEX

#ifdef TARGET_STAX
const uint8_t charUuidTX[BLE_UUID_SIZE]   = {0x72,0x65,0x67,0x64,0x65,0x4c,0x01,0x00,0x04,0x60,0x97,0x2c,0x00,0x34,0xd6,0x13};
const uint8_t charUuidRX[BLE_UUID_SIZE]   = {0x72,0x65,0x67,0x64,0x65,0x4c,0x02,0x00,0x04,0x60,0x97,0x2c,0x00,0x34,0xd6,0x13};
const uint8_t charUuidRX2[BLE_UUID_SIZE]  = {0x72,0x65,0x67,0x64,0x65,0x4c,0x03,0x00,0x04,0x60,0x97,0x2c,0x00,0x34,0xd6,0x13};
#else // !TARGET_FLEX && !TARGET_STAX

const uint8_t charUuidTX[BLE_UUID_SIZE]   = {0x72,0x65,0x67,0x64,0x65,0x4c,0x01,0x00,0x04,0x00,0x97,0x2c,0x00,0x34,0xd6,0x13};
const uint8_t charUuidRX[BLE_UUID_SIZE]   = {0x72,0x65,0x67,0x64,0x65,0x4c,0x02,0x00,0x04,0x00,0x97,0x2c,0x00,0x34,0xd6,0x13};
const uint8_t charUuidRX2[BLE_UUID_SIZE]  = {0x72,0x65,0x67,0x64,0x65,0x4c,0x03,0x00,0x04,0x00,0x97,0x2c,0x00,0x34,0xd6,0x13};

#endif // ! TARGET_STAX
#endif // ! TARGET_FLEX
// clang-format on
static uint8_t BLE_LEDGER_apdu_buffer[OS_IO_BUFFER_SIZE + 1];

/* Exported variables --------------------------------------------------------*/
const ble_profile_info_t BLE_LEDGER_PROFILE_apdu_info = {
    .type = BLE_LEDGER_PROFILE_APDU,

    .service_uuid
    = {.type = BLE_GATT_UUID_TYPE_128,
// clang-format off
#ifdef TARGET_FLEX
                     .value = {0x72,0x65,0x67,0x64,0x65,0x4c,0x00,0x00,0x04,0x30,0x97,0x2c,0x00,0x34,0xd6,0x13}},
#else // !TARGET_FLEX

#ifdef TARGET_STAX
                     .value = {0x72,0x65,0x67,0x64,0x65,0x4c,0x00,0x00,0x04,0x60,0x97,0x2c,0x00,0x34,0xd6,0x13}},
#else // !TARGET_FLEX && !TARGET_STAX

                     .value = {0x72,0x65,0x67,0x64,0x65,0x4c,0x00,0x00,0x04,0x00,0x97,0x2c,0x00,0x34,0xd6,0x13}},
#endif // ! TARGET_STAX
#endif // ! TARGET_FLEX
    // clang-format on

    .init            = BLE_LEDGER_PROFILE_apdu_init,
    .create_db       = BLE_LEDGER_PROFILE_apdu_create_db,
    .handle_in_range = BLE_LEDGER_PROFILE_apdu_handle_in_range,

    .connection_evt        = BLE_LEDGER_PROFILE_apdu_connection_evt,
    .connection_update_evt = BLE_LEDGER_PROFILE_apdu_connection_update_evt,
    .encryption_changed    = BLE_LEDGER_PROFILE_apdu_encryption_changed,

    .att_modified     = BLE_LEDGER_PROFILE_apdu_att_modified,
    .write_permit_req = BLE_LEDGER_PROFILE_apdu_write_permit_req,
    .mtu_changed      = BLE_LEDGER_PROFILE_apdu_mtu_changed,

    .write_rsp_ack       = BLE_LEDGER_PROFILE_apdu_write_rsp_ack,
    .update_char_val_ack = BLE_LEDGER_PROFILE_apdu_update_char_value_ack,

    .send_packet = BLE_LEDGER_PROFILE_apdu_send_packet,
    .is_busy     = BLE_LEDGER_PROFILE_apdu_is_busy,

    .data_ready = BLE_LEDGER_PROFILE_apdu_data_ready,

    .setting = BLE_LEDGER_PROFILE_apdu_setting,

    .cookie = &ledger_apdu_profile_handle,
};

/* Private functions ---------------------------------------------------------*/
static void notify_chunk(ledger_ble_profile_apdu_handle_t *handle)
{
    if ((handle->protocol_data.tx_chunk_length >= 2) && sizeof(ble_ledger_protocol_chunk_buffer) >= handle->protocol_data.tx_chunk_length) {
        ble_aci_gatt_forge_cmd_update_char_value(handle->cmd_data,
                                                 handle->gatt_service_handle,
                                                 handle->gatt_notification_characteristic_handle,
                                                 0,
                                                 handle->protocol_data.tx_chunk_length - 2,
                                                 &ble_ledger_protocol_chunk_buffer[2]);
    }
}

static void check_transfer_mode(ledger_ble_profile_apdu_handle_t *handle)
{
    if (handle->transfer_mode_enabled != handle->transfer_mode_wanted_enabled) {
        LOG_IO("LEDGER_BLE_set_transfer_mode %d\n", handle->transfer_mode_wanted_enabled);
    }

    if ((handle->transfer_mode_enabled == 0) && (handle->transfer_mode_wanted_enabled != 0)) {
        handle->send_response = true;
        U2BE_ENCODE(handle->resp, 0, SWO_SUCCESS);
    }

    handle->transfer_mode_enabled = handle->transfer_mode_wanted_enabled;
}

/* Exported functions --------------------------------------------------------*/
ble_profile_status_t BLE_LEDGER_PROFILE_apdu_init(ble_cmd_data_t *cmd_data, void *cookie)
{
    ble_profile_status_t status = BLE_PROFILE_STATUS_BAD_PARAMETERS;
    if (!cmd_data || !cookie) {
        goto error;
    }

    ledger_ble_profile_apdu_handle_t *handle = (ledger_ble_profile_apdu_handle_t *) PIC(cookie);

    memset(handle, 0, sizeof(ledger_ble_profile_apdu_handle_t));
    handle->cmd_data = cmd_data;

    memset(&handle->protocol_data, 0, sizeof(handle->protocol_data));
    handle->protocol_data.mtu                  = BLE_ATT_DEFAULT_MTU - 1;

    ledger_protocol_result_t result = LEDGER_PROTOCOL_init(&handle->protocol_data, OS_IO_PACKET_TYPE_BLE_APDU);
    if (result == LP_SUCCESS) {
        handle->gatt_service_handle                     = BLE_GATT_INVALID_HANDLE;
        handle->gatt_notification_characteristic_handle = BLE_GATT_INVALID_HANDLE;
        handle->gatt_write_characteristic_handle        = BLE_GATT_INVALID_HANDLE;
        handle->gatt_write_cmd_characteristic_handle    = BLE_GATT_INVALID_HANDLE;

        handle->create_db_step = CREATE_DB_STEP_IDLE;
        status = BLE_PROFILE_STATUS_OK;
    }

error:
    return status;
}

ble_profile_status_t BLE_LEDGER_PROFILE_apdu_create_db(uint8_t *hci_buffer, uint16_t length, void *cookie)
{
    ble_profile_status_t status = BLE_PROFILE_STATUS_BAD_PARAMETERS;
    if (!hci_buffer || !cookie) {
        goto error;
    }

    ledger_ble_profile_apdu_handle_t *handle = (ledger_ble_profile_apdu_handle_t *) PIC(cookie);

    if (handle->create_db_step == CREATE_DB_STEP_IDLE) {
        LOG_IO("APDU PROFILE INIT START\n");
    }
    else if ((length >= 2) && (handle->create_db_step == CREATE_DB_STEP_ADD_SERVICE)) {
        handle->gatt_service_handle = U2LE(hci_buffer, 4);
        LOG_IO("APDU PROFILE GATT service handle        : %04X\n", handle->gatt_service_handle);
    }
    else if ((length >= 2)
             && (handle->create_db_step == CREATE_DB_STEP_ADD_NOTIFICATION_CHARACTERISTIC)) {
        handle->gatt_notification_characteristic_handle = U2LE(hci_buffer, 4);
        LOG_IO("APDU PROFILE GATT notif char handle     : %04X\n",
               handle->gatt_notification_characteristic_handle);
    }
    else if ((length >= 2) && (handle->create_db_step == CREATE_DB_STEP_ADD_WRITE_CHARACTERISTIC)) {
        handle->gatt_write_characteristic_handle = U2LE(hci_buffer, 4);
        LOG_IO("APDU PROFILE GATT write char handle     : %04X\n",
               handle->gatt_write_characteristic_handle);
    }
    else if ((length >= 2)
             && (handle->create_db_step == CREATE_DB_STEP_ADD_WRITE_COMMAND_CHARACTERISTIC)) {
        handle->gatt_write_cmd_characteristic_handle = U2LE(hci_buffer, 4);
        LOG_IO("APDU PROFILE GATT write cmd char handle : %04X\n",
               handle->gatt_write_cmd_characteristic_handle);
    }

    handle->create_db_step++;

    switch (handle->create_db_step) {
        case CREATE_DB_STEP_ADD_SERVICE:
            ble_aci_gatt_forge_cmd_add_service(handle->cmd_data,
                                               BLE_LEDGER_PROFILE_apdu_info.service_uuid.type,
                                               BLE_LEDGER_PROFILE_apdu_info.service_uuid.value,
                                               BLE_GATT_PRIMARY_SERVICE,
                                               BLE_GATT_MAX_SERVICE_RECORDS);
            status = BLE_PROFILE_STATUS_OK;
            break;

        case CREATE_DB_STEP_ADD_NOTIFICATION_CHARACTERISTIC: {
            ble_cmd_add_char_data_t data;
            data.service_handle       = handle->gatt_service_handle;
            data.char_uuid_type       = BLE_GATT_UUID_TYPE_128;
            data.char_uuid            = charUuidTX;
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

        case CREATE_DB_STEP_ADD_WRITE_CHARACTERISTIC: {
            ble_cmd_add_char_data_t data;
            data.service_handle       = handle->gatt_service_handle;
            data.char_uuid_type       = BLE_GATT_UUID_TYPE_128;
            data.char_uuid            = charUuidRX;
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

        case CREATE_DB_STEP_ADD_WRITE_COMMAND_CHARACTERISTIC: {
            ble_cmd_add_char_data_t data;
            data.service_handle       = handle->gatt_service_handle;
            data.char_uuid_type       = BLE_GATT_UUID_TYPE_128;
            data.char_uuid            = charUuidRX2;
            data.char_value_length    = BLE_ATT_MAX_MTU_SIZE;
            data.char_properties      = BLE_CHAR_PROP_WRITE_WITHOUT_RESP;
            data.security_permissions = BLE_GATT_ATTR_PERMISSION_AUTHEN_WRITE;
            data.gatt_evt_mask        = BLE_GATT_NOTIFY_ATTRIBUTE_WRITE;
            data.enc_key_size         = BLE_GAP_MAX_ENCRYPTION_KEY_SIZE;
            data.is_variable          = 1;
            ble_aci_gatt_forge_cmd_add_char(handle->cmd_data, &data);
            status = BLE_PROFILE_STATUS_OK;
            break;
        }

        case CREATE_DB_STEP_END:
            LOG_IO("APDU PROFILE INIT END\n");
            status = BLE_PROFILE_STATUS_OK_PROCEDURE_END;
            break;

        default:
            break;
    }

error:
    return status;
}

uint8_t BLE_LEDGER_PROFILE_apdu_handle_in_range(uint16_t gatt_handle, void *cookie)
{
    if (!cookie) {
        return 0;
    }

    ledger_ble_profile_apdu_handle_t *handle = (ledger_ble_profile_apdu_handle_t *) PIC(cookie);

    if ((gatt_handle >= handle->gatt_service_handle)
        && (gatt_handle <= handle->gatt_write_cmd_characteristic_handle + 1)) {
        return 1;
    }

    return 0;
}

void BLE_LEDGER_PROFILE_apdu_connection_evt(ble_connection_t *connection, void *cookie)
{
    if (!cookie || !connection) {
        return;
    }

    ledger_ble_profile_apdu_handle_t *handle = (ledger_ble_profile_apdu_handle_t *) PIC(cookie);

    handle->notifications_enabled = 0;
    handle->protocol_data.mtu     = BLE_ATT_DEFAULT_MTU - 1;
    handle->mtu_negotiated        = 0;
    handle->connection_updated    = 0;
    handle->wait_write_resp_ack   = 0;
    handle->transfer_mode_enabled = 0;
    handle->send_response             = false;
    handle->connection            = connection;
}

void BLE_LEDGER_PROFILE_apdu_connection_update_evt(ble_connection_t *connection, void *cookie)
{
    if (!cookie || !connection) {
        return;
    }

    ledger_ble_profile_apdu_handle_t *handle = (ledger_ble_profile_apdu_handle_t *) PIC(cookie);

    handle->connection = connection;
}

void BLE_LEDGER_PROFILE_apdu_encryption_changed(bool encrypted, void *cookie)
{
    if (!cookie) {
        return;
    }

    ledger_ble_profile_apdu_handle_t *handle = (ledger_ble_profile_apdu_handle_t *) PIC(cookie);

    handle->link_is_encrypted = encrypted;
}

ble_profile_status_t BLE_LEDGER_PROFILE_apdu_att_modified(uint8_t *hci_buffer, uint16_t length, void *cookie)
{
    if (!hci_buffer || !cookie || (length < 10)) {
        return BLE_PROFILE_STATUS_BAD_PARAMETERS;
    }

    ble_profile_status_t status = BLE_PROFILE_STATUS_BAD_PARAMETERS;
    ledger_ble_profile_apdu_handle_t *handle = (ledger_ble_profile_apdu_handle_t *) PIC(cookie);

    uint16_t connection_handle = U2LE(hci_buffer, 2);
    uint16_t att_handle        = U2LE(hci_buffer, 4);
    uint16_t offset            = U2LE(hci_buffer, 6);
    uint16_t att_data_length   = U2LE(hci_buffer, 8);

    if ((att_handle == handle->gatt_notification_characteristic_handle + 2)
        && (att_data_length == 2) && (offset == 0)) {
        // Peer device registering/unregistering for notifications
        if (U2LE(hci_buffer, 10) != 0) {
            LOG_IO("REGISTERED FOR NOTIFICATIONS\n");
            handle->notifications_enabled = 1;
            if (!handle->mtu_negotiated) {
                ble_aci_gatt_forge_cmd_exchange_config(handle->cmd_data, connection_handle);
                status = BLE_PROFILE_STATUS_OK_AND_SEND_PACKET;
            }
        }
        else {
            LOG_IO("NOT REGISTERED FOR NOTIFICATIONS\n");
            handle->notifications_enabled = 0;
            status = BLE_PROFILE_STATUS_OK;
        }
    }
    else if ((att_handle == handle->gatt_write_cmd_characteristic_handle + 1)
             && (handle->notifications_enabled) && (handle->link_is_encrypted)
             && (att_data_length)) {
        LOG_IO("WRITE CMD %d\n", length - 8);
        hci_buffer[8] = 0xDE;
        hci_buffer[9] = 0xF1;
        ledger_protocol_result_t result = LEDGER_PROTOCOL_rx(&handle->protocol_data, &hci_buffer[8], length - 8, ble_ledger_protocol_chunk_buffer, sizeof(ble_ledger_protocol_chunk_buffer), BLE_LEDGER_apdu_buffer, sizeof(BLE_LEDGER_apdu_buffer), handle->protocol_data.mtu);
        if (result != LP_SUCCESS) {
            goto error;
        }

        if (handle->protocol_data.rx_apdu_status == APDU_STATUS_COMPLETE) {
            check_transfer_mode(handle);
            if (handle->transfer_mode_enabled) {
                if (U2BE(handle->resp, 0) != SWO_SUCCESS) {
                    LOG_IO("Transfer failed 0x%04x\n", U2BE(handle->resp, 0));
                    handle->transfer_mode_wanted_enabled = 0;
                    check_transfer_mode(handle);
                    status = BLE_PROFILE_STATUS_OK;
                }
                else if (handle->send_response) {
                    result = LEDGER_PROTOCOL_tx(&handle->protocol_data, handle->resp, sizeof(handle->resp), ble_ledger_protocol_chunk_buffer, sizeof(ble_ledger_protocol_chunk_buffer), handle->protocol_data.mtu);
                    if (result != LP_SUCCESS) {
                        goto error;
                    }
                    handle->send_response = false;
                    notify_chunk(handle);
                    status = BLE_PROFILE_STATUS_OK_AND_SEND_PACKET;
                }
            }
        }
        else if (handle->protocol_data.tx_chunk_length >= 2) {
            notify_chunk(handle);
            status = BLE_PROFILE_STATUS_OK_AND_SEND_PACKET;
        }
    } else {
        status = BLE_PROFILE_STATUS_OK;
    }

error:
    return status;
}

ble_profile_status_t BLE_LEDGER_PROFILE_apdu_write_permit_req(uint8_t *hci_buffer, uint16_t length, void *cookie)
{
    ble_profile_status_t status = BLE_PROFILE_STATUS_BAD_PARAMETERS;
    if (!hci_buffer || !cookie || (length < 7)) {
        goto error;
    }

    ledger_ble_profile_apdu_handle_t *handle = (ledger_ble_profile_apdu_handle_t *) PIC(cookie);

    uint16_t connection_handle = U2LE(hci_buffer, 2);
    uint16_t att_handle        = U2LE(hci_buffer, 4);
    uint8_t  data_length       = hci_buffer[6];

    handle->wait_write_resp_ack = 1;

    if ((att_handle == handle->gatt_write_characteristic_handle + 1)
        && (handle->notifications_enabled) && (handle->link_is_encrypted) && (data_length)) {
        hci_buffer[5] = 0xDE;
        hci_buffer[6] = 0xF1;
        ledger_protocol_result_t result = LEDGER_PROTOCOL_rx(&handle->protocol_data, &hci_buffer[5], length - 5, ble_ledger_protocol_chunk_buffer, sizeof(ble_ledger_protocol_chunk_buffer), BLE_LEDGER_apdu_buffer, sizeof(BLE_LEDGER_apdu_buffer), handle->protocol_data.mtu);
        if (result != LP_SUCCESS) {
            goto error;
        }
        ble_aci_gatt_forge_cmd_write_resp(handle->cmd_data,
                                          connection_handle,
                                          att_handle,
                                          0,
                                          HCI_SUCCESS_ERR_CODE,
                                          data_length,
                                          &hci_buffer[7]);
    }
    else {
        LOG_IO("ATT WRITE %04X %d bytes\n", att_handle, data_length);
        handle->protocol_data.tx_chunk_length = 0;
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

ble_profile_status_t BLE_LEDGER_PROFILE_apdu_mtu_changed(uint16_t mtu, void *cookie)
{
    ble_profile_status_t status = BLE_PROFILE_STATUS_BAD_PARAMETERS;
    if (!cookie) {
        goto error;
    }

    if ((mtu > 1) && (mtu <= BLE_ATT_MAX_MTU_SIZE)) {
        ledger_ble_profile_apdu_handle_t *handle = (ledger_ble_profile_apdu_handle_t *) PIC(cookie);

        handle->protocol_data.mtu = mtu - 1;
        handle->mtu_negotiated    = 1;
        status = BLE_PROFILE_STATUS_OK;
    }

error:
    return status;
}

ble_profile_status_t BLE_LEDGER_PROFILE_apdu_write_rsp_ack(void *cookie)
{
    ble_profile_status_t status = BLE_PROFILE_STATUS_BAD_PARAMETERS;
    if (!cookie) {
        goto error;
    }

    ledger_ble_profile_apdu_handle_t *handle = (ledger_ble_profile_apdu_handle_t *) PIC(cookie);

    if (handle->wait_write_resp_ack != 0) {
        handle->wait_write_resp_ack = 0;
        if (handle->protocol_data.tx_chunk_length >= 2) {
            notify_chunk(handle);
            status = BLE_PROFILE_STATUS_OK_AND_SEND_PACKET;
        }
    } else {
        status = BLE_PROFILE_STATUS_OK;
    }

error:
    return status;
}

ble_profile_status_t BLE_LEDGER_PROFILE_apdu_update_char_value_ack(void *cookie)
{
    ble_profile_status_t status = BLE_PROFILE_STATUS_BAD_PARAMETERS;
    if (!cookie) {
        goto error;
    }

    ledger_ble_profile_apdu_handle_t *handle = (ledger_ble_profile_apdu_handle_t *) PIC(cookie);

    handle->protocol_data.tx_chunk_length = 0;
    if (!handle->transfer_mode_enabled) {
        if (handle->protocol_data.tx_apdu_buffer) {
            ledger_protocol_result_t result = LEDGER_PROTOCOL_tx(&handle->protocol_data, NULL, 0, ble_ledger_protocol_chunk_buffer, sizeof(ble_ledger_protocol_chunk_buffer), handle->protocol_data.mtu);
            if (result != LP_SUCCESS) {
                goto error;
            }
            notify_chunk(handle);
            status = BLE_PROFILE_STATUS_OK_AND_SEND_PACKET;
        }
        if (!handle->protocol_data.tx_apdu_buffer) {
            handle->protocol_data.tx_chunk_length = 0;
            handle->state                         = LEDGER_BLE_PROFILE_APDU_STATE_IDLE;
            if ((!handle->connection_updated)
                && (handle->connection->conn_interval > BLE_SLAVE_CONN_INTERVAL_MIN)) {
                handle->connection_updated = 1;
                ble_aci_l2cap_forge_cmd_connection_parameter_update(
                    handle->cmd_data,
                    handle->connection->connection_handle,
                    CONN_INTERVAL_MIN,
                    CONN_INTERVAL_MIN,
                    handle->connection->conn_latency,
                    handle->connection->supervision_timeout);
                status = BLE_PROFILE_STATUS_OK_AND_SEND_PACKET;
            }
        }
    } else {
        status = BLE_PROFILE_STATUS_OK;
    }

error:
    return status;
}

ble_profile_status_t BLE_LEDGER_PROFILE_apdu_send_packet(const uint8_t *packet, uint16_t length, void *cookie)
{
    ble_profile_status_t status = BLE_PROFILE_STATUS_BAD_PARAMETERS;
    if (!packet || !cookie) {
        goto error;
    }

    ledger_ble_profile_apdu_handle_t *handle = (ledger_ble_profile_apdu_handle_t *) PIC(cookie);

    if ((handle->transfer_mode_enabled != 0) && (length == 2)) {
        handle->send_response = true;
        handle->resp[0]     = packet[0];
        handle->resp[1]     = packet[1];
        if (handle->protocol_data.rx_apdu_length) {
            ledger_protocol_result_t result = LEDGER_PROTOCOL_tx(&handle->protocol_data, packet, length, ble_ledger_protocol_chunk_buffer, sizeof(ble_ledger_protocol_chunk_buffer), handle->protocol_data.mtu);
            if (result != LP_SUCCESS) {
                goto error;
            }
            notify_chunk(handle);
            status = BLE_PROFILE_STATUS_OK_AND_SEND_PACKET;
        } else {
            status = BLE_PROFILE_STATUS_OK;
        }
    }
    else {
        if ((handle->send_response == true) && (U2BE(handle->resp, 0) != SWO_SUCCESS)) {
            ledger_protocol_result_t result = LEDGER_PROTOCOL_tx(&handle->protocol_data, handle->resp, sizeof(handle->resp), ble_ledger_protocol_chunk_buffer, sizeof(ble_ledger_protocol_chunk_buffer), handle->protocol_data.mtu);
            if (result != LP_SUCCESS) {
                goto error;
            }
        }
        else {
            ledger_protocol_result_t result = LEDGER_PROTOCOL_tx(&handle->protocol_data, packet, length, ble_ledger_protocol_chunk_buffer, sizeof(ble_ledger_protocol_chunk_buffer), handle->protocol_data.mtu);
            if (result != LP_SUCCESS) {
                goto error;
            }
            handle->state = LEDGER_BLE_PROFILE_APDU_STATE_IDLE;
        }
        handle->send_response = false;

        if (handle->wait_write_resp_ack == 0) {
            notify_chunk(handle);
            status = BLE_PROFILE_STATUS_OK_AND_SEND_PACKET;
        } else {
            status = BLE_PROFILE_STATUS_OK;
        }
    }

error:
    return status;
}

bool BLE_LEDGER_PROFILE_apdu_is_busy(void *cookie)
{
    ledger_ble_profile_apdu_handle_t *handle = (ledger_ble_profile_apdu_handle_t *) PIC(cookie);
    bool                              busy   = false;

    if (handle->state == LEDGER_BLE_PROFILE_APDU_BUSY) {
        busy = true;
    }

    return busy;
}

int32_t BLE_LEDGER_PROFILE_apdu_data_ready(uint8_t *buffer, uint16_t max_length, void *cookie)
{
    int32_t status = 0;

    if (!buffer || !cookie) {
        return -1;
    }

    ledger_ble_profile_apdu_handle_t *handle = (ledger_ble_profile_apdu_handle_t *) PIC(cookie);

    PRINTF("BLE_LEDGER_PROFILE_apdu_data_ready %d\n", handle->protocol_data.rx_apdu_status);
    if (handle->protocol_data.rx_apdu_status == APDU_STATUS_COMPLETE) {
        if ((max_length < handle->protocol_data.rx_apdu_length)
            || (sizeof(BLE_LEDGER_apdu_buffer) < handle->protocol_data.rx_apdu_length)) {
            status = -1;
        }
        else {
            memmove(
                buffer, BLE_LEDGER_apdu_buffer, handle->protocol_data.rx_apdu_length);
            status = handle->protocol_data.rx_apdu_length;
        }
        handle->protocol_data.rx_apdu_status = APDU_STATUS_WAITING;
        handle->protocol_data.rx_apdu_length = 0;
    }

    return status;
}

void BLE_LEDGER_PROFILE_apdu_setting(uint32_t id, uint8_t *buffer, uint16_t length, void *cookie)
{
    if ((id == BLE_LEDGER_PROFILE_APDU_SETTING_ID_TRANSFER_MODE) && (buffer) && (length == 1) && (cookie)) {
        ledger_ble_profile_apdu_handle_t *handle = (ledger_ble_profile_apdu_handle_t *) PIC(cookie);
        handle->transfer_mode_wanted_enabled     = buffer[0];
    }
}
