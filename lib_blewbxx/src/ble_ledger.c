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
#include <string.h>

#include "os.h"
#include "os_helpers.h"
#include "os_settings.h"
#include "seproxyhal_protocol.h"
#include "os_io.h"
#include "os_io_seph_cmd.h"


#include "lcx_rng.h"

#include "ble_cmd.h"
#include "ble_ledger.h"
#include "ble_ledger_types.h"
#include "ble_ledger_profile_apdu.h"

/* Private enumerations ------------------------------------------------------*/
typedef enum ble_state_e {
    BLE_STATE_IDLE = 0,
    BLE_STATE_INITIALIZED,
    BLE_STATE_STARTING,
    BLE_STATE_RUNNING,
    BLE_STATE_START_ADVERTISING,
    BLE_STATE_STOPPING,
} ble_state_t;

typedef enum ble_init_step_e {
    BLE_INIT_STEP_IDLE,
    BLE_INIT_STEP_RESET,
    BLE_INIT_STEP_STATIC_ADDRESS,
    BLE_INIT_STEP_GATT_INIT,
    BLE_INIT_STEP_GAP_INIT,
    BLE_INIT_STEP_SET_IO_CAPABILITIES,
    BLE_INIT_STEP_SET_AUTH_REQUIREMENTS,
    BLE_INIT_STEP_PROFILE_INIT,
    BLE_INIT_STEP_PROFILE_CREATE_DB,
    BLE_INIT_STEP_SET_TX_POWER_LEVEL,
    BLE_INIT_STEP_CLEAR_PAIRING,
    BLE_INIT_STEP_START_ADVERTISING,
    BLE_INIT_STEP_END,
} ble_init_step_t;

typedef enum ble_start_adv_step_e {
    BLE_START_ADV_STEP_IDLE,
    BLE_START_ADV_STEP_STOP_ADV,
    BLE_START_ADV_STEP_SET_ADV_DATAS,
    BLE_START_ADV_STEP_SET_SCAN_RSP_DATAS,
    BLE_START_ADV_STEP_SET_GAP_DEVICE_NAME,
    BLE_START_ADV_STEP_START_ADV,
    BLE_START_ADV_STEP_END,
} ble_start_adv_step_t;

typedef enum ble_stopping_step_e {
    BLE_STOPPING_STEP_IDLE,
    BLE_STOPPING_STEP_DISCONNECT,
    BLE_STOPPING_STEP_STOP_ADV,
    BLE_STOPPING_STEP_RESET,
    BLE_STOPPING_STEP_END,
} ble_stopping_step_t;

/* Private defines------------------------------------------------------------*/

/* Private types, structures, unions -----------------------------------------*/

typedef struct ble_ledger_data_s {
    // General
    bool             enabled;
    ble_state_t         state;
    char                device_name[BLE_GAP_MAX_LOCAL_NAME_LENGTH];
    char                device_name_length;
    uint8_t             random_address[BLE_CONFIG_DATA_RANDOM_ADDRESS_LEN];
    uint8_t             nb_of_profile;
    ble_profile_info_t *profile[2];
    uint16_t            profiles;
    uint8_t             ble_ready;

    // Init
    ble_init_step_t init_step;

    // Start advertising
    ble_start_adv_step_t start_adv_step;
    uint8_t                    stop_after_start;

    // Stopping
    ble_stopping_step_t stopping_step;
    uint8_t             start_after_stop;

    // HCI
    ble_cmd_data_t cmd_data;

    // GAP
    uint16_t         gap_service_handle;
    uint16_t         gap_device_name_characteristic_handle;
    uint16_t         gap_appearance_characteristic_handle;
    bool          advertising_enabled;
    ble_connection_t connection;
    uint16_t         pairing_code;
    uint8_t          pairing_in_progress;

    // PAIRING
    uint8_t clear_pairing;

} ble_ledger_data_t;

#ifdef HAVE_PRINTF
#define LOG_IO PRINTF
// #define LOG_IO(...)
#else  // !HAVE_PRINTF
#define LOG_IO(...)
#endif  // !HAVE_PRINTF

/* Private macros-------------------------------------------------------------*/

/* Private functions prototypes ----------------------------------------------*/
// Init
static void get_device_name(void);
static void start_mngr(uint8_t *hci_buffer, uint16_t length);

// Advertising
static void start_advertising_mngr(uint16_t opcode);
static void stopping_mngr(uint16_t opcode);

#ifdef HAVE_INAPP_BLE_PAIRING
// Pairing UX
static void ask_user_pairing_numeric_comparison(uint32_t code);
static void rsp_user_pairing_numeric_comparison(unsigned int status);
static void ask_user_pairing_passkey(void);
static void rsp_user_pairing_passkey(unsigned int status);
static void end_pairing_ux(uint8_t pairing_ok);
#endif  // HAVE_INAPP_BLE_PAIRING

// HCI
static uint32_t send_hci_packet(uint32_t timeout_ms);

/* Exported variables --------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static ble_ledger_data_t ble_ledger_data;
static os_io_init_ble_t  ble_ledger_init_data;

/* Private functions ---------------------------------------------------------*/
static void get_device_name(void)
{
    memset(ble_ledger_data.device_name, 0, sizeof(ble_ledger_data.device_name));
    ble_ledger_data.device_name_length = os_setting_get(OS_SETTING_BLUETOOTH_NAME,
                                                        (uint8_t *) ble_ledger_data.device_name,
                                                        sizeof(ble_ledger_data.device_name));
}

static void start_mngr(uint8_t *hci_buffer, uint16_t length)
{

    if (ble_ledger_data.init_step == BLE_INIT_STEP_IDLE) {
        LOG_IO("INIT START\n");
    }
    else if (hci_buffer && length) {
        uint16_t opcode = U2LE(hci_buffer, 1);
        if ((ble_ledger_data.cmd_data.hci_cmd_opcode != 0xFFFF)
            && (opcode != ble_ledger_data.cmd_data.hci_cmd_opcode)) {
            // Unexpected event => TODO_IO
            return;
        }
        if ((length >= 10) && (ble_ledger_data.init_step == BLE_INIT_STEP_GAP_INIT)) {
            ble_ledger_data.gap_service_handle                    = U2LE(hci_buffer, 4);
            ble_ledger_data.gap_device_name_characteristic_handle = U2LE(hci_buffer, 6);
            ble_ledger_data.gap_appearance_characteristic_handle  = U2LE(hci_buffer, 8);
            LOG_IO("GAP service handle          : %04X\n", ble_ledger_data.gap_service_handle);
            LOG_IO("GAP device name char handle : %04X\n",
                   ble_ledger_data.gap_device_name_characteristic_handle);
            LOG_IO("GAP appearance char handle  : %04X\n",
                   ble_ledger_data.gap_appearance_characteristic_handle);
        }
        if (ble_ledger_data.init_step == BLE_INIT_STEP_START_ADVERTISING) {
            start_advertising_mngr(opcode);
            if (ble_ledger_data.start_adv_step != BLE_START_ADV_STEP_END) {
                return;
            }
        }
        if (ble_ledger_data.init_step == BLE_INIT_STEP_CLEAR_PAIRING - 1) {
            if (!ble_ledger_data.clear_pairing) {
                ble_ledger_data.init_step++;
            }
        }
    }

    ble_ledger_data.init_step++;

    switch (ble_ledger_data.init_step) {
        case BLE_INIT_STEP_RESET:
            ble_hci_forge_cmd_reset(&ble_ledger_data.cmd_data);
            send_hci_packet(0);
            break;

        case BLE_INIT_STEP_STATIC_ADDRESS:
            ble_aci_hal_forge_cmd_write_config_data(&ble_ledger_data.cmd_data,
                                                    BLE_CONFIG_DATA_RANDOM_ADDRESS_OFFSET,
                                                    BLE_CONFIG_DATA_RANDOM_ADDRESS_LEN,
                                                    ble_ledger_data.random_address);
            send_hci_packet(0);
            break;

        case BLE_INIT_STEP_GATT_INIT:
            ble_aci_gatt_forge_cmd_init(&ble_ledger_data.cmd_data);
            send_hci_packet(0);
            break;

        case BLE_INIT_STEP_GAP_INIT:
            ble_aci_gap_forge_cmd_init(&ble_ledger_data.cmd_data,
                                       BLE_GAP_PERIPHERAL_ROLE,
                                       BLE_GAP_PRIVACY_DISABLED,
                                       sizeof(ble_ledger_data.device_name));
            send_hci_packet(0);
            break;

        case BLE_INIT_STEP_SET_IO_CAPABILITIES:
            ble_aci_gap_forge_cmd_set_io_capability(&ble_ledger_data.cmd_data,
                                                    BLE_GAP_IO_CAP_DISPLAY_YES_NO);
            send_hci_packet(0);
            break;

        case BLE_INIT_STEP_SET_AUTH_REQUIREMENTS: {
            ble_cmd_set_auth_req_data_t data;
            data.bonding_mode                   = BLE_GAP_BONDING_ENABLED;
            data.mitm_mode                      = BLE_GAP_MITM_PROTECTION_REQUIRED;
            data.sc_support                     = 1;
            data.key_press_notification_support = BLE_GAP_KEYPRESS_NOT_SUPPORTED;
            data.min_encryption_key_size        = BLE_GAP_MIN_ENCRYPTION_KEY_SIZE;
            data.max_encryption_key_size        = BLE_GAP_MAX_ENCRYPTION_KEY_SIZE;
            data.use_fixed_pin                  = BLE_GAP_USE_FIXED_PIN_FOR_PAIRING_FORBIDDEN;
            data.fixed_pin                      = 0;
            data.identity_address_type          = BLE_GAP_STATIC_RANDOM_ADDR;
            ble_aci_gap_forge_cmd_set_authentication_requirement(&ble_ledger_data.cmd_data, &data);
            send_hci_packet(0);
            break;
        }

        case BLE_INIT_STEP_PROFILE_INIT:
        case BLE_INIT_STEP_PROFILE_CREATE_DB: {
            ble_profile_info_t *profile_info = NULL;
            if (ble_ledger_data.init_step == BLE_INIT_STEP_PROFILE_INIT) {
                for (uint8_t index = 0; index < ble_ledger_data.nb_of_profile; index++) {
                    profile_info = ble_ledger_data.profile[index];
                    if (profile_info->init) {
                        ((ble_profile_init_t) PIC(profile_info->init))(&ble_ledger_data.cmd_data,
                                                                       profile_info->cookie);
                    }
                }
                ble_ledger_data.init_step++;
            }
            uint8_t db_creation_complete = 1;
            for (uint8_t index = 0; index < ble_ledger_data.nb_of_profile; index++) {
                profile_info = ble_ledger_data.profile[index];
                if (profile_info->create_db) {
                    if (((ble_profile_create_db_t) PIC(profile_info->create_db))(
                            hci_buffer, length, profile_info->cookie)
                        != BLE_PROFILE_STATUS_OK_PROCEDURE_END) {
                        ble_ledger_data.init_step--;
                        send_hci_packet(0);
                        db_creation_complete = 0;
                    }
                }
            }
            if (db_creation_complete) {
                start_mngr(hci_buffer, length);
            }
            break;
        }

        case BLE_INIT_STEP_SET_TX_POWER_LEVEL:
            ble_aci_hal_forge_cmd_set_tx_power_level(&ble_ledger_data.cmd_data,
                                                     1,  // High power
#if defined(TARGET_STAX) || defined(TARGET_FLEX) || defined(TARGET_APEX)
                                                     0x19);  // 0 dBm
#else                                                        // !TARGET_STAX && !TARGET_FLEX && !TARGET_APEX
                                                     0x07);  // -14.1 dBm
#endif                                                       // TARGET_STAX || TARGET_FLEX || TARGET_APEX
            send_hci_packet(0);
            break;

        case BLE_INIT_STEP_CLEAR_PAIRING:
            ble_aci_gap_forge_cmd_clear_security_db(&ble_ledger_data.cmd_data);
            send_hci_packet(0);
            break;

        case BLE_INIT_STEP_START_ADVERTISING:
            ble_ledger_data.cmd_data.hci_cmd_opcode = 0xFFFF;
            ble_ledger_data.start_adv_step          = BLE_START_ADV_STEP_IDLE;
            start_advertising_mngr(0);
            break;

        case BLE_INIT_STEP_END:
            LOG_IO("INIT END\n");
            ble_ledger_data.state = BLE_STATE_RUNNING;
            ble_ledger_data.clear_pairing = 0;
            break;

        default:
            break;
    }
}

static void start_advertising_mngr(uint16_t opcode)
{
    uint8_t buffer[31] = {0};
    uint8_t index = 0;

    if ((ble_ledger_data.cmd_data.hci_cmd_opcode != 0xFFFF)
        && (opcode != ble_ledger_data.cmd_data.hci_cmd_opcode)) {
        // Unexpected event => TODO_IO
        return;
    }

    if (ble_ledger_data.connection.connection_handle != 0xFFFF) {
        LOG_IO("START ADVERTISING ABORTED : connected\n");
        ble_ledger_data.state = BLE_STATE_RUNNING;
        return;
    }

    if (ble_ledger_data.start_adv_step == BLE_START_ADV_STEP_IDLE) {
        LOG_IO("START ADVERTISING START\n");
        ble_ledger_data.start_after_stop = 0;
    }

    if (ble_ledger_data.start_adv_step == BLE_START_ADV_STEP_STOP_ADV - 1) {
        if (!ble_ledger_data.advertising_enabled) {
            ble_ledger_data.start_adv_step++;
        }
    }

    if (ble_ledger_data.start_adv_step == (BLE_START_ADV_STEP_START_ADV - 1)) {
        if ((os_setting_get(OS_SETTING_PLANEMODE, NULL, 0)) || ble_ledger_data.stop_after_start) {
            LOG_IO("START ADVERTISING ABORTED\n");
            ble_ledger_data.state = BLE_STATE_RUNNING;
            BLE_LEDGER_stop();
            return;
        }
    }

    ble_ledger_data.start_adv_step++;

    // Be sure to retrieve the actual name value and length
    get_device_name();

    switch (ble_ledger_data.start_adv_step) {
        case BLE_START_ADV_STEP_STOP_ADV:
            ble_aci_gap_forge_cmd_set_non_discoverable(&ble_ledger_data.cmd_data);
            send_hci_packet(0);
            break;

        case BLE_START_ADV_STEP_SET_ADV_DATAS:
            ble_ledger_data.advertising_enabled = false;
            // Flags
            buffer[index++] = 2;
            buffer[index++] = BLE_AD_TYPE_FLAGS;
            buffer[index++] = BLE_AD_TYPE_FLAG_BIT_BR_EDR_NOT_SUPPORTED
                              | BLE_AD_TYPE_FLAG_BIT_LE_GENERAL_DISCOVERABLE_MODE;

            buffer[index++] = ble_ledger_data.device_name_length + 1;
            buffer[index++] = BLE_AD_TYPE_COMPLETE_LOCAL_NAME;
            memcpy(&buffer[index], ble_ledger_data.device_name, ble_ledger_data.device_name_length);
            index += ble_ledger_data.device_name_length;

            ble_aci_gap_forge_cmd_update_adv_data(&ble_ledger_data.cmd_data, index, buffer);
            send_hci_packet(0);
            break;

        case BLE_START_ADV_STEP_SET_SCAN_RSP_DATAS: {
            ble_profile_info_t *profile_info = NULL;
            // TODO_IO : should handle every profile in the future
            if (ble_ledger_data.nb_of_profile) {
                profile_info = ble_ledger_data.profile[0];
                // Incomplete List of 128-bit Service UUIDs
                buffer[index++] = 16 + 1;
                buffer[index++] = BLE_AD_AD_TYPE_128_BIT_SERV_UUID;
                memcpy(&buffer[index], (uint8_t *) PIC(profile_info->service_uuid.value), 16);
                index += 16;
            }

            // Slave Connection Interval Range
            buffer[index++] = 5;
            buffer[index++] = BLE_AD_AD_TYPE_SLAVE_CONN_INTERVAL;
            buffer[index++] = BLE_SLAVE_CONN_INTERVAL_MIN;
            buffer[index++] = 0;
            buffer[index++] = BLE_SLAVE_CONN_INTERVAL_MAX;
            buffer[index++] = 0;

            ble_hci_forge_cmd_set_scan_response_data(&ble_ledger_data.cmd_data, index, buffer);
            send_hci_packet(0);
            break;
        }

        case BLE_START_ADV_STEP_SET_GAP_DEVICE_NAME:
            ble_aci_gatt_forge_cmd_update_char_value(
                &ble_ledger_data.cmd_data,
                ble_ledger_data.gap_service_handle,
                ble_ledger_data.gap_device_name_characteristic_handle,
                0,
                ble_ledger_data.device_name_length,
                (uint8_t *) ble_ledger_data.device_name);
            send_hci_packet(0);
            break;

        case BLE_START_ADV_STEP_START_ADV:
            buffer[0] = BLE_AD_TYPE_COMPLETE_LOCAL_NAME;
            memcpy(&buffer[1], ble_ledger_data.device_name, ble_ledger_data.device_name_length);

            ble_cmd_set_discoverable_data_t data;
            data.advertising_type          = BLE_GAP_ADV_IND;
            data.advertising_interval_min  = BLE_ADVERTISING_INTERVAL_MIN;
            data.advertising_interval_max  = BLE_ADVERTISING_INTERVAL_MAX;
            data.own_address_type          = BLE_GAP_RANDOM_ADDR_TYPE;
            data.advertising_filter_policy = BLE_GAP_NO_WHITE_LIST_USE;
            data.local_name_length         = ble_ledger_data.device_name_length + 1;
            data.local_name                = buffer;
            data.service_uuid_length       = 0;
            data.service_uuid_list         = NULL;
            data.slave_conn_interval_min   = 0;
            data.slave_conn_interval_max   = 0;
            ble_aci_gap_forge_cmd_set_discoverable(&ble_ledger_data.cmd_data, &data);
            send_hci_packet(0);
            break;

        case BLE_START_ADV_STEP_END:
            LOG_IO("START ADVERTISING END\n");
            ble_ledger_data.advertising_enabled = true;
            ble_ledger_data.state               = BLE_STATE_RUNNING;
            break;

        default:
            break;
    }
}

static void stopping_mngr(uint16_t opcode)
{
    if ((ble_ledger_data.cmd_data.hci_cmd_opcode != 0xFFFF)
        && (opcode != ble_ledger_data.cmd_data.hci_cmd_opcode)) {
        // Unexpected event => TODO_IO
        return;
    }

    if (ble_ledger_data.stopping_step == BLE_STOPPING_STEP_IDLE) {
        LOG_IO("STOPPING START\n");
        ble_ledger_data.stop_after_start = 0;
    }

    if ((ble_ledger_data.stopping_step == BLE_STOPPING_STEP_DISCONNECT - 1)
        && (ble_ledger_data.connection.connection_handle == 0xFFFF)) {
        ble_ledger_data.stopping_step++;
    }

    if ((ble_ledger_data.stopping_step == BLE_STOPPING_STEP_STOP_ADV - 1)
        && (!ble_ledger_data.advertising_enabled)) {
        ble_ledger_data.stopping_step++;
    }

    ble_ledger_data.stopping_step++;

    switch (ble_ledger_data.stopping_step) {
        case BLE_STOPPING_STEP_DISCONNECT:
#ifdef HAVE_INAPP_BLE_PAIRING
            end_pairing_ux(BOLOS_UX_ASYNCHMODAL_PAIRING_STATUS_FAILED);
#endif  // HAVE_INAPP_BLE_PAIRING
            ble_hci_forge_cmd_disconnect(&ble_ledger_data.cmd_data,
                                         ble_ledger_data.connection.connection_handle);
            send_hci_packet(0);
            break;

        case BLE_STOPPING_STEP_STOP_ADV:
            ble_aci_gap_forge_cmd_set_non_discoverable(&ble_ledger_data.cmd_data);
            send_hci_packet(0);
            break;

        case BLE_STOPPING_STEP_RESET:
            ble_hci_forge_cmd_reset(&ble_ledger_data.cmd_data);
            send_hci_packet(0);
            break;

        case BLE_STOPPING_STEP_END:
            LOG_IO("STOPPING END\n");
            ble_ledger_data.state = BLE_STATE_INITIALIZED;
            if (ble_ledger_data.start_after_stop) {
                BLE_LEDGER_start();
            }
            break;

        default:
            break;
    }
}

#ifdef HAVE_INAPP_BLE_PAIRING
static void ask_user_pairing_numeric_comparison(uint32_t code)
{
    ble_ledger_data.pairing_in_progress = 1;

    char pairing_info[16] = {0};
    SPRINTF(pairing_info, "%06d", (unsigned int) code);

    os_io_ux_cmd_ble_pairing_request(BOLOS_UX_ASYNCHMODAL_PAIRING_REQUEST_NUMCOMP, pairing_info, 6);
}

static void rsp_user_pairing_numeric_comparison(unsigned int status)
{
    if (status == BOLOS_UX_OK) {
        end_pairing_ux(BOLOS_UX_ASYNCHMODAL_PAIRING_STATUS_CONFIRM_CODE_YES);
        ble_aci_gap_forge_cmd_numeric_comparison_value_confirm_yesno(
            &ble_ledger_data.cmd_data, ble_ledger_data.connection.connection_handle, 1);
    }
    else if (status == BOLOS_UX_IGNORE) {
        ble_ledger_data.pairing_in_progress = 0;
        ble_aci_gap_forge_cmd_numeric_comparison_value_confirm_yesno(
            &ble_ledger_data.cmd_data, ble_ledger_data.connection.connection_handle, 0);
    }
    else {
        end_pairing_ux(BOLOS_UX_ASYNCHMODAL_PAIRING_STATUS_CONFIRM_CODE_NO);
        ble_aci_gap_forge_cmd_numeric_comparison_value_confirm_yesno(
            &ble_ledger_data.cmd_data, ble_ledger_data.connection.connection_handle, 0);
    }
    send_hci_packet(0);
}

static void ask_user_pairing_passkey(void)
{
    ble_ledger_data.pairing_in_progress = 2;
    ble_ledger_data.pairing_code        = cx_rng_u32_range_func(0, 1000000, cx_rng_u32);

    char pairing_info[16] = {0};
    SPRINTF(pairing_info, "%06d", ble_ledger_data.pairing_code);

    os_io_ux_cmd_ble_pairing_request(BOLOS_UX_ASYNCHMODAL_PAIRING_REQUEST_PASSKEY, pairing_info, 6);
}

static void rsp_user_pairing_passkey(unsigned int status)
{
    if (status == BOLOS_UX_OK) {
        end_pairing_ux(BOLOS_UX_ASYNCHMODAL_PAIRING_STATUS_ACCEPT_PASSKEY);
        ble_ledger_data.pairing_code = cx_rng_u32_range_func(0, 1000000, cx_rng_u32);
        ble_aci_gap_forge_cmd_pass_key_resp(&ble_ledger_data.cmd_data,
                                            ble_ledger_data.connection.connection_handle,
                                            ble_ledger_data.pairing_code);
        send_hci_packet(0);
    }
    else if (status == BOLOS_UX_IGNORE) {
        ble_ledger_data.pairing_in_progress = 0;
    }
    else {
        end_pairing_ux(BOLOS_UX_ASYNCHMODAL_PAIRING_STATUS_CANCEL_PASSKEY);
    }
}

static void end_pairing_ux(uint8_t pairing_ok)
{
    LOG_IO("end_pairing_ux : %d (%d)\n", pairing_ok, ble_ledger_data.pairing_in_progress);
    if (ble_ledger_data.pairing_in_progress) {
        os_io_ux_cmd_ble_pairing_status(pairing_ok);
    }
}
#endif  // HAVE_INAPP_BLE_PAIRING

static int32_t hci_evt_cmd_complete(uint8_t *buffer, uint16_t length)
{
    int32_t status = -1;
    if (length < 3) {
        goto error;
    }

    uint16_t opcode = U2LE(buffer, 1);

    if (ble_ledger_data.state == BLE_STATE_STARTING) {
        start_mngr(buffer, length);
    }
    else if (ble_ledger_data.state == BLE_STATE_START_ADVERTISING) {
        start_advertising_mngr(opcode);
    }
    else if (ble_ledger_data.state == BLE_STATE_STOPPING) {
        stopping_mngr(opcode);
    }
    else if (opcode == ACI_GATT_WRITE_RESP_CMD_CODE) {
        ble_profile_info_t *profile_info = NULL;
        for (uint8_t index = 0; index < ble_ledger_data.nb_of_profile; index++) {
            profile_info = ble_ledger_data.profile[index];
            if (profile_info->write_rsp_ack) {
                ble_profile_status_t ble_status = ((ble_profile_write_rsp_ack_t) PIC(profile_info->write_rsp_ack))(
                    profile_info->cookie);
                if (ble_status == BLE_PROFILE_STATUS_OK_AND_SEND_PACKET) {
                    send_hci_packet(0);
                }
            }
        }
    }
    else if (opcode == ACI_GATT_UPDATE_CHAR_VALUE_CMD_CODE) {
        ble_profile_info_t *profile_info = NULL;
        for (uint8_t index = 0; index < ble_ledger_data.nb_of_profile; index++) {
            profile_info = ble_ledger_data.profile[index];
            if (profile_info->update_char_val_ack) {
                ble_profile_status_t ble_status = ((ble_profile_update_char_value_ack_t) PIC(
                    profile_info->update_char_val_ack))(profile_info->cookie);
                if (ble_status == BLE_PROFILE_STATUS_OK_AND_SEND_PACKET) {
                    send_hci_packet(0);
                }
            }
        }
    }
    else if (opcode == ACI_GAP_NUMERIC_COMPARISON_VALUE_CONFIRM_YESNO_CMD_CODE) {
        LOG_IO("ACI_GAP_NUMERIC_COMPARISON_VALUE_CONFIRM_YESNO\n");
    }
    else if (opcode == ACI_GAP_CLEAR_SECURITY_DB_CMD_CODE) {
        LOG_IO("ACI_GAP_CLEAR_SECURITY_DB\n");
    }
    else if (opcode == ACI_GATT_CONFIRM_INDICATION_CMD_CODE) {
        LOG_IO("ACI_GATT_CONFIRM_INDICATION\n");
    }
    else {
        LOG_IO("HCI EVT CMD COMPLETE 0x%04X\n", opcode);
    }
    status = 0;

error:
    return status;
}

static int32_t hci_evt_le_meta_evt(uint8_t *buffer, uint16_t length)
{
    int32_t status = -1;
    if (!length) {
        goto error;
    }

    switch (buffer[0]) {
        case HCI_LE_CONNECTION_COMPLETE_SUBEVT_CODE:
            if (length < 19) {
                goto error;
            }
            ble_ledger_data.connection.connection_handle   = U2LE(buffer, 2);
            ble_ledger_data.connection.role_slave          = buffer[4];
            ble_ledger_data.connection.peer_address_random = buffer[5];
            memcpy(ble_ledger_data.connection.peer_address, &buffer[6], 6);
            ble_ledger_data.connection.conn_interval         = U2LE(buffer, 12);
            ble_ledger_data.connection.conn_latency          = U2LE(buffer, 14);
            ble_ledger_data.connection.supervision_timeout   = U2LE(buffer, 16);
            ble_ledger_data.connection.master_clock_accuracy = buffer[18];
            ble_ledger_data.connection.encrypted             = false;
            LOG_IO("LE CONNECTION COMPLETE %04X - %04X- %04X- %04X\n",
                   ble_ledger_data.connection.connection_handle,
                   ble_ledger_data.connection.conn_interval,
                   ble_ledger_data.connection.conn_latency,
                   ble_ledger_data.connection.supervision_timeout);
            ble_ledger_data.advertising_enabled = 0;

            for (uint8_t index = 0; index < ble_ledger_data.nb_of_profile; index++) {
                ble_profile_info_t *profile_info = ble_ledger_data.profile[index];
                if (profile_info->connection_evt) {
                    ((ble_profile_connection_evt_t) PIC(profile_info->connection_evt))(
                        &ble_ledger_data.connection, profile_info->cookie);
                }
            }
            break;

        case HCI_LE_CONNECTION_UPDATE_COMPLETE_SUBEVT_CODE:
            if (length < 10) {
                goto error;
            }
            ble_ledger_data.connection.connection_handle   = U2LE(buffer, 2);
            ble_ledger_data.connection.conn_interval       = U2LE(buffer, 4);
            ble_ledger_data.connection.conn_latency        = U2LE(buffer, 6);
            ble_ledger_data.connection.supervision_timeout = U2LE(buffer, 8);
            LOG_IO("LE CONNECTION UPDATE %04X - %04X- %04X- %04X\n",
                   ble_ledger_data.connection.connection_handle,
                   ble_ledger_data.connection.conn_interval,
                   ble_ledger_data.connection.conn_latency,
                   ble_ledger_data.connection.supervision_timeout);

            for (uint8_t index = 0; index < ble_ledger_data.nb_of_profile; index++) {
                ble_profile_info_t *profile_info = ble_ledger_data.profile[index];
                if (profile_info->connection_update_evt) {
                    ((ble_profile_connection_update_evt_t) PIC(
                        profile_info->connection_update_evt))(&ble_ledger_data.connection,
                                                              profile_info->cookie);
                }
            }
            break;

        case HCI_LE_DATA_LENGTH_CHANGE_SUBEVT_CODE:
            if (length < 11) {
                goto error;
            }
            if (U2LE(buffer, 1) == ble_ledger_data.connection.connection_handle) {
                ble_ledger_data.connection.max_tx_octets = U2LE(buffer, 3);
                ble_ledger_data.connection.max_tx_time   = U2LE(buffer, 5);
                ble_ledger_data.connection.max_rx_octets = U2LE(buffer, 7);
                ble_ledger_data.connection.max_rx_time   = U2LE(buffer, 9);
                LOG_IO("LE DATA LENGTH CHANGE %04X - %04X- %04X- %04X\n",
                       ble_ledger_data.connection.max_tx_octets,
                       ble_ledger_data.connection.max_tx_time,
                       ble_ledger_data.connection.max_rx_octets,
                       ble_ledger_data.connection.max_rx_time);
            }
            break;

        case HCI_LE_PHY_UPDATE_COMPLETE_SUBEVT_CODE:
            if (length < 6) {
                goto error;
            }
            if (U2LE(buffer, 2) == ble_ledger_data.connection.connection_handle) {
                ble_ledger_data.connection.tx_phy = buffer[4];
                ble_ledger_data.connection.rx_phy = buffer[5];
                LOG_IO("LE PHY UPDATE %02X - %02X\n",
                       ble_ledger_data.connection.tx_phy,
                       ble_ledger_data.connection.rx_phy);
            }
            break;

        default:
            LOG_IO("HCI LE META 0x%02X\n", buffer[0]);
            break;
    }
    status = 0;
error:
    return status;
}

static int32_t hci_evt_vendor(uint8_t *buffer, uint16_t length)
{
    int32_t status = -1;
    size_t evt_min_size = sizeof(uint16_t) + sizeof(ble_ledger_data.connection.connection_handle);
    if (length < evt_min_size) {
        goto error;
    }

    uint8_t offset = 0;
    uint16_t opcode = U2LE(buffer, offset);
    offset += sizeof(opcode);

    uint16_t handle = U2LE(buffer, offset);
    offset += sizeof(handle);
    if (handle != ble_ledger_data.connection.connection_handle) {
        goto error;
    }

    switch (opcode) {
#ifdef HAVE_INAPP_BLE_PAIRING
        case ACI_GAP_PAIRING_COMPLETE_VSEVT_CODE:
            LOG_IO("PAIRING");
            if (length < evt_min_size + 1) {
                goto error;
            }
            switch (buffer[offset++]) {
                case SMP_PAIRING_STATUS_SUCCESS:
                    LOG_IO(" SUCCESS\n");
                    end_pairing_ux(BOLOS_UX_ASYNCHMODAL_PAIRING_STATUS_SUCCESS);
                    break;

                case SMP_PAIRING_STATUS_SMP_TIMEOUT:
                    LOG_IO(" TIMEOUT\n");
                    end_pairing_ux(BOLOS_UX_ASYNCHMODAL_PAIRING_STATUS_TIMEOUT);
                    break;

                case SMP_PAIRING_STATUS_PAIRING_FAILED:
                    if (length < evt_min_size + 1 + 1) {
                        goto error;
                    }
                    LOG_IO(" FAILED : %02X\n", buffer[offset]);
                    if (buffer[offset] == 0x08) {  // UNSPECIFIED_REASON
                        end_pairing_ux(BOLOS_UX_ASYNCHMODAL_PAIRING_STATUS_CANCELLED_FROM_REMOTE);
                    }
                    else {
                        end_pairing_ux(BOLOS_UX_ASYNCHMODAL_PAIRING_STATUS_FAILED);
                    }
                    break;

                default:
                    end_pairing_ux(BOLOS_UX_ASYNCHMODAL_PAIRING_STATUS_FAILED);
                    break;
            }
            ble_ledger_data.pairing_in_progress = 0;
            break;

        case ACI_GAP_PASS_KEY_REQ_VSEVT_CODE:
            LOG_IO("PASSKEY REQ\n");
            ask_user_pairing_passkey();
            break;

        case ACI_GAP_NUMERIC_COMPARISON_VALUE_VSEVT_CODE:
            if (length < evt_min_size + sizeof(uint32_t)) {
                goto error;
            }
            LOG_IO("NUMERIC COMP : %d\n", U4LE(buffer, offset));
            ask_user_pairing_numeric_comparison(U4LE(buffer, offset));
            break;
#endif  // HAVE_INAPP_BLE_PAIRING

        case ACI_L2CAP_CONNECTION_UPDATE_RESP_VSEVT_CODE: {
            if (length < evt_min_size + 1) {
                goto error;
            }
            LOG_IO("CONNECTION UPDATE RESP %d\n", buffer[offset]);
            break;
        }

        case ACI_GATT_ATTRIBUTE_MODIFIED_VSEVT_CODE:
        case ACI_GATT_WRITE_PERMIT_REQ_VSEVT_CODE: {
            if (length < (evt_min_size + (3 * sizeof(uint16_t)))) {
                goto error;
            }
            uint16_t            att_handle   = U2LE(buffer, offset);
            offset += sizeof(att_handle);
            uint8_t             profil_found = 0;
            ble_profile_info_t *profile_info = NULL;
            for (uint8_t index = 0; index < ble_ledger_data.nb_of_profile; index++) {
                profile_info = ble_ledger_data.profile[index];
                if ((profile_info->handle_in_range)
                    && (((ble_profile_handle_in_range_t) PIC(profile_info->handle_in_range))(
                        att_handle, profile_info->cookie))) {
                    ble_profile_status_t ble_status = BLE_PROFILE_STATUS_OK;
                    if (opcode == ACI_GATT_ATTRIBUTE_MODIFIED_VSEVT_CODE) {
                        if (profile_info->att_modified) {
                            ble_status = ((ble_profile_att_modified_t) PIC(profile_info->att_modified))(
                                buffer, length, profile_info->cookie);
                        }
                    }
                    else if (opcode == ACI_GATT_WRITE_PERMIT_REQ_VSEVT_CODE) {
                        if (profile_info->write_permit_req) {
                            ble_status = ((ble_profile_write_permit_req_t) PIC(
                                profile_info->write_permit_req))(
                                buffer, length, profile_info->cookie);
                        }
                    }
                    if (ble_status == BLE_PROFILE_STATUS_OK_AND_SEND_PACKET) {
                        send_hci_packet(0);
                    }
                    profil_found = 1;
                }
            }
            if ((!profil_found) && (opcode == ACI_GATT_ATTRIBUTE_MODIFIED_VSEVT_CODE)) {
                LOG_IO("ATT MODIFIED %04X %d bytes at offset %d\n",
                       att_handle,
                       U2LE(buffer, offset + sizeof(uint16_t)),
                       U2LE(buffer, offset));
                UNUSED(offset);
            }
            break;
        }

        case ACI_ATT_EXCHANGE_MTU_RESP_VSEVT_CODE:
            if (length < evt_min_size + sizeof(uint16_t)) {
                goto error;
            }
            uint16_t mtu = U2LE(buffer, offset);
            LOG_IO("MTU : %d\n", mtu);
            ble_profile_info_t *profile_info = NULL;
            for (uint8_t index = 0; index < ble_ledger_data.nb_of_profile; index++) {
                profile_info = ble_ledger_data.profile[index];
                if (profile_info->mtu_changed) {
                    ble_profile_status_t ble_status = BLE_PROFILE_STATUS_OK;
                    ble_status         = ((ble_profile_mtu_changed_t) PIC(profile_info->mtu_changed))(
                        mtu, profile_info->cookie);
                    if (ble_status == BLE_PROFILE_STATUS_OK_AND_SEND_PACKET) {
                        send_hci_packet(0);
                    }
                }
            }
            break;

        case ACI_GATT_INDICATION_VSEVT_CODE:
            LOG_IO("INDICATION EVT\n");
            ble_aci_gatt_forge_cmd_confirm_indication(&ble_ledger_data.cmd_data,
                                                      ble_ledger_data.connection.connection_handle);
            send_hci_packet(0);
            break;

        case ACI_GATT_PROC_COMPLETE_VSEVT_CODE:
            LOG_IO("PROCEDURE COMPLETE\n");
            break;

        case ACI_GATT_PROC_TIMEOUT_VSEVT_CODE:
            LOG_IO("PROCEDURE TIMEOUT\n");
#ifdef HAVE_INAPP_BLE_PAIRING
            end_pairing_ux(BOLOS_UX_ASYNCHMODAL_PAIRING_STATUS_FAILED);
#endif  // HAVE_INAPP_BLE_PAIRING
            if (ble_ledger_data.connection.connection_handle != 0xFFFF) {
                ble_hci_forge_cmd_disconnect(&ble_ledger_data.cmd_data,
                                             ble_ledger_data.connection.connection_handle);
                send_hci_packet(0);
            }
            break;

        default:
            LOG_IO("HCI VENDOR 0x%04X\n", opcode);
            break;
    }
    status = 0;

error:
    return status;
}

static uint32_t send_hci_packet(uint32_t timeout_ms)
{
    uint8_t hdr[3] = {0};
    UNUSED(timeout_ms);
    if (sizeof(ble_ledger_data.cmd_data.hci_cmd_buffer) < ble_ledger_data.cmd_data.hci_cmd_buffer_length) {
        return 1;
    }


    hdr[0] = SEPROXYHAL_TAG_BLE_SEND;
    U2BE_ENCODE(hdr, 1, ble_ledger_data.cmd_data.hci_cmd_buffer_length);
    os_io_tx_cmd(OS_IO_PACKET_TYPE_SEPH, hdr, sizeof(hdr), NULL);
    os_io_tx_cmd(OS_IO_PACKET_TYPE_SEPH,
                 ble_ledger_data.cmd_data.hci_cmd_buffer,
                 ble_ledger_data.cmd_data.hci_cmd_buffer_length,
                 NULL);

    return 0;
}

static int32_t is_data_ready(uint8_t *buffer, uint16_t max_length)
{
    int32_t status = 0;

    for (uint8_t index = 0; index < ble_ledger_data.nb_of_profile; index++) {
        ble_profile_info_t *profile_info = ble_ledger_data.profile[index];
        if (profile_info->data_ready) {
            status = ((ble_profile_data_ready_t) PIC(profile_info->data_ready))(
                buffer, max_length, profile_info->cookie);
            if (status > 0) {
                return status;
            }
        }
    }

    return status;
}

/* Exported functions --------------------------------------------------------*/
void BLE_LEDGER_init(os_io_init_ble_t *init, uint8_t force)
{
    if (!init) {
        return;
    }

    if ((force) || (ble_ledger_data.state == BLE_STATE_IDLE)) {
        // First time BLE is started or forced to reinit
        uint8_t random_address[6];
        memcpy(random_address, ble_ledger_data.random_address, sizeof(random_address));
        memset(&ble_ledger_data, 0, sizeof(ble_ledger_data));
        if ((random_address[5] == 0xDE) && (random_address[4] == 0xF1)) {
            memcpy(ble_ledger_data.random_address,
                   random_address,
                   sizeof(ble_ledger_data.random_address));
        }
        else {
            LEDGER_BLE_get_mac_address(ble_ledger_data.random_address);
        }
        LOG_IO("BLE_LEDGER_init deep\n");
        ble_ledger_data.state = BLE_STATE_INITIALIZED;
    }
    else {
        LOG_IO("BLE_LEDGER_init\n");
    }

    memcpy(&ble_ledger_init_data, init, sizeof(ble_ledger_init_data));
}

void BLE_LEDGER_start(void)
{
    LOG_IO("BLE_LEDGER_start\n");

    if ((ble_ledger_data.state == BLE_STATE_INITIALIZED)
        || (ble_ledger_data.profiles != ble_ledger_init_data.profile_mask)) {
        // BLE is not initialized
        // or wanted classes have changed
        ble_ledger_data.cmd_data.hci_cmd_opcode      = 0xFFFF;
        ble_ledger_data.state                        = BLE_STATE_STARTING;
        ble_ledger_data.init_step                    = BLE_INIT_STEP_IDLE;
        ble_ledger_data.nb_of_profile                = 0;
        ble_ledger_data.connection.connection_handle = 0xFFFF;
        if (ble_ledger_init_data.profile_mask & BLE_LEDGER_PROFILE_APDU) {
            LOG_IO("APDU ");
            ble_ledger_data.profile[ble_ledger_data.nb_of_profile++]
                = (ble_profile_info_t *) PIC(&BLE_LEDGER_PROFILE_apdu_info);
        }
#ifdef HAVE_IO_U2F
        else if (ble_ledger_init_data.profile_mask & BLE_LEDGER_PROFILE_U2F) {
            LOG_IO("U2F ");
            // ble_ledger_data.profile[ble_ledger_data.nb_of_profile++] =
            // (ble_profile_info_t*)PIC(&BLE_LEDGER_PROFILE_u2f_info);
        }
        LOG_IO("\n");
#endif  // HAVE_IO_U2F
        ble_ledger_data.profiles = ble_ledger_init_data.profile_mask;
        start_mngr(NULL, 0);
    }
    else if (ble_ledger_data.state == BLE_STATE_RUNNING) {
        ble_ledger_data.cmd_data.hci_cmd_opcode = 0xFFFF;
        ble_ledger_data.state                   = BLE_STATE_START_ADVERTISING;
        ble_ledger_data.start_adv_step          = BLE_START_ADV_STEP_IDLE;
        start_advertising_mngr(0);
    }
    else if (ble_ledger_data.state == BLE_STATE_STOPPING) {
        ble_ledger_data.start_after_stop = 1;
    }
}

void BLE_LEDGER_stop(void)
{
    LOG_IO("BLE_LEDGER_stop\n");

    if (ble_ledger_data.state == BLE_STATE_RUNNING) {
        ble_ledger_data.cmd_data.hci_cmd_opcode = 0xFFFF;
        ble_ledger_data.state                   = BLE_STATE_STOPPING;
        ble_ledger_data.stopping_step           = BLE_STOPPING_STEP_IDLE;
        stopping_mngr(0);
    }
    else if ((ble_ledger_data.state == BLE_STATE_STARTING)
             || (ble_ledger_data.state == BLE_STATE_START_ADVERTISING)) {
        ble_ledger_data.stop_after_start = 1;
    }
}

void BLE_LEDGER_reset_pairings(void)
{
    if (ble_ledger_data.state == BLE_STATE_RUNNING) {
        ble_ledger_data.clear_pairing = 1;
        BLE_LEDGER_stop();
        BLE_LEDGER_start();
    }
}

void BLE_LEDGER_accept_pairing(uint8_t status)
{
    if (ble_ledger_data.state == BLE_STATE_RUNNING) {
        if (ble_ledger_data.pairing_in_progress == 1) {
            rsp_user_pairing_numeric_comparison(status);
        }
        else if (ble_ledger_data.pairing_in_progress == 2) {
            rsp_user_pairing_passkey(status);
        }
    }
}

void BLE_LEDGER_name_changed(void)
{
    if ((ble_ledger_data.state == BLE_STATE_RUNNING)
        && (ble_ledger_data.connection.connection_handle == 0xFFFF)) {
        BLE_LEDGER_stop();
        BLE_LEDGER_start();
    }
}

int BLE_LEDGER_rx_seph_evt(uint8_t *seph_buffer,
                           uint16_t seph_buffer_length,
                           uint8_t *apdu_buffer,
                           uint16_t apdu_buffer_max_length)
{
    int32_t status = -1;

    LOG_IO("BLE_LEDGER_rx_seph_evt state(%u)\n", ble_ledger_data.state);
    seph_t seph = {0};
    if (seph_buffer_length == 0) {
        goto error;
    }
    // Offset 0 is for the seph packet type, not handled here so skip it
    seph_buffer_length -= 1;
    seph_buffer++;
    if (seph_buffer_length && !seph_parse_header(seph_buffer, seph_buffer_length, &seph)) {
        goto error;
    }
    if ((ble_ledger_data.state == BLE_STATE_STOPPING) ||
        (ble_ledger_data.state == BLE_STATE_STARTING) ||
        (ble_ledger_data.state == BLE_STATE_RUNNING) ||
        (ble_ledger_data.state == BLE_STATE_START_ADVERTISING)) {
        // Check enough room for packet type and event type
        size_t seph_header_size = seph_get_header_size();
        uint8_t packet_type = 0;
        ble_hci_event_packet_t event = {0};
        size_t event_pkt_size = seph_header_size + sizeof(packet_type) + sizeof(event.code) + sizeof(event.length);
        // Count at least one event param
        if (seph_buffer_length < event_pkt_size + sizeof(uint8_t)) {
            status= -1;
            goto error;
        }
        uint8_t offset = seph_header_size;
        packet_type = seph_buffer[offset++];

        if (packet_type == HCI_EVENT_PKT_TYPE) {
            event.code = seph_buffer[offset++];
            event.length = seph_buffer[offset++];
            if (seph_buffer_length < event_pkt_size + event.length) {
                goto error;
            }
            switch (event.code) {
                case HCI_DISCONNECTION_COMPLETE_EVT_CODE:
                    if (seph_buffer_length < (event_pkt_size + 4)) {
                        goto error;
                    }
                    else {
                        LOG_IO("HCI DISCONNECTION COMPLETE code %02X\n", seph_buffer[offset + 2]);
                        ble_ledger_data.connection.connection_handle = 0xFFFF;
                        ble_ledger_data.advertising_enabled          = false;
                        ble_ledger_data.connection.encrypted         = false;
    #ifdef HAVE_INAPP_BLE_PAIRING
                        end_pairing_ux(BOLOS_UX_ASYNCHMODAL_PAIRING_STATUS_FAILED);
    #endif  // HAVE_INAPP_BLE_PAIRING
                        if (ble_ledger_data.state == BLE_STATE_RUNNING) {
                            ble_ledger_data.cmd_data.hci_cmd_opcode = 0xFFFF;
                            ble_ledger_data.state                   = BLE_STATE_START_ADVERTISING;
                            ble_ledger_data.start_adv_step          = BLE_START_ADV_STEP_IDLE;
                            start_advertising_mngr(0);
                        }
                    }
                    status = 0;
                    break;

                case HCI_ENCRYPTION_CHANGE_EVT_CODE:
                    if (seph_buffer_length  < (event_pkt_size + 4)) {
                        goto error;
                    }
                    uint16_t handle = U2LE(seph_buffer, offset + 1);
                    uint8_t enabled = seph_buffer[offset + 3];
                    if (handle == ble_ledger_data.connection.connection_handle) {
                        if (enabled) {
                            LOG_IO("Link encrypted\n");
                            ble_ledger_data.connection.encrypted = true;
                        }
                        else {
                            LOG_IO("Link not encrypted\n");
                            ble_ledger_data.connection.encrypted = false;
                        }
                        ble_profile_info_t *profile_info = NULL;
                        for (uint8_t index = 0; index < ble_ledger_data.nb_of_profile; index++) {
                            profile_info = ble_ledger_data.profile[index];
                            if (profile_info->encryption_changed) {
                                ((ble_profile_encryption_changed_t) PIC(
                                    profile_info->encryption_changed))(
                                    ble_ledger_data.connection.encrypted, profile_info->cookie);
                            }
                        }
                    }
                    else {
                        LOG_IO("HCI ENCRYPTION CHANGE EVT %d on connection handle 0x%02X\n",
                               enabled,
                               handle);
                    }
                    status = 0;
                    break;

                case HCI_COMMAND_COMPLETE_EVT_CODE:
                    status = hci_evt_cmd_complete(&seph_buffer[offset], event.length);
                    break;

                case HCI_COMMAND_STATUS_EVT_CODE:
                    if (seph_buffer_length < (event_pkt_size + 4)) {
                        goto error;
                    }
                    uint16_t opcode = U2LE(seph_buffer, offset + 2);
                    LOG_IO("HCI COMMAND_STATUS %d - num %d - op %04X\n",
                           seph_buffer[offset],
                           seph_buffer[offset + 1],
                           opcode);
                    if (ble_ledger_data.state == BLE_STATE_STOPPING) {
                        stopping_mngr(opcode);
                    }
                    status = 0;
                    break;

                case HCI_ENCRYPTION_KEY_REFRESH_COMPLETE_EVT_CODE:
                    LOG_IO("HCI KEY_REFRESH_COMPLETE\n");
                    status = 0;
                    break;

                case HCI_LE_META_EVT_CODE:
                    status = hci_evt_le_meta_evt(&seph_buffer[offset], event.length);
                    break;

                case HCI_VENDOR_SPECIFIC_DEBUG_EVT_CODE:
                    if (seph_buffer_length < (event_pkt_size + 2)) {
                        goto error;
                    }
                    else {
                        status = hci_evt_vendor(&seph_buffer[offset], event.length);
                        if (!status) {
                            status = is_data_ready(apdu_buffer, apdu_buffer_max_length);
                        }
                    }
                    break;

                default:
                    break;
            }
        }
    } else {
        status = -1;
    }

error:
    return status;
}

uint32_t BLE_LEDGER_send(uint8_t        profile_type,
                         const uint8_t *packet,
                         uint16_t       packet_length,
                         uint32_t       timeout_ms)
{
    uint32_t status = 1;

    if (ble_ledger_data.state == BLE_STATE_RUNNING) {
        for (uint8_t index = 0; index < ble_ledger_data.nb_of_profile; index++) {
            ble_profile_info_t *profile_info = ble_ledger_data.profile[index];
            uint8_t ble_status   = BLE_PROFILE_STATUS_OK;
            if (profile_info->type == profile_type) {
                ble_status = ((ble_profile_send_packet_t) PIC(profile_info->send_packet))(
                    packet, packet_length, profile_info->cookie);
            if (ble_status == BLE_PROFILE_STATUS_OK_AND_SEND_PACKET) {
                status = send_hci_packet(timeout_ms);
                    break;
                }
            }
        }
    }

    return status;
}


bool BLE_LEDGER_is_busy(void)
{
    bool busy = false;

    if (ble_ledger_data.state == BLE_STATE_RUNNING) {
        for (uint8_t index = 0; index < ble_ledger_data.nb_of_profile; index++) {
            ble_profile_info_t *profile_info = ble_ledger_data.profile[index];
            if (profile_info->is_busy) {
                busy = ((ble_profile_is_busy_t) PIC(profile_info->is_busy))(profile_info->cookie);
                if (busy) {
                    break;
                }
            }
        }
    }

    return busy;
}

void BLE_LEDGER_setting(uint32_t profile_id, uint32_t setting_id, uint8_t *buffer, uint16_t length)
{
    if (ble_ledger_data.state == BLE_STATE_RUNNING) {
        ble_profile_info_t *profile_info = NULL;
        for (uint8_t index = 0; index < ble_ledger_data.nb_of_profile; index++) {
            profile_info = ble_ledger_data.profile[index];
            if ((profile_info->type == profile_id) && (profile_info->setting)) {
                ((ble_profile_setting_t) PIC(profile_info->setting))(
                    setting_id, buffer, length, profile_info->cookie);
            }
        }
    }
}
