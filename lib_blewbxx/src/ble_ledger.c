/* @BANNER@ */

/* Includes ------------------------------------------------------------------*/
#include "os.h"
#include "os_settings.h"
#include "seproxyhal_protocol.h"
#include "os_io.h"
#include "os_io_seph_cmd.h"

#include <string.h>

#include "lcx_rng.h"

#include "ble_cmd.h"
#include "ble_ledger.h"
#include "ble_ledger_types.h"
#include "ble_ledger_profile_apdu.h"

#pragma GCC diagnostic ignored "-Wcast-qual"

/* Private enumerations ------------------------------------------------------*/
typedef enum {
    BLE_STATE_INITIALIZING,
    BLE_STATE_INITIALIZED,
    BLE_STATE_CONFIGURE_ADVERTISING,
    BLE_STATE_CONNECTING,
    BLE_STATE_CONNECTED,
    BLE_STATE_DISCONNECTING,
} ble_state_t;

typedef enum {
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
    BLE_INIT_STEP_CONFIGURE_ADVERTISING,
    BLE_INIT_STEP_END,
} ble_init_step_t;

typedef enum {
    BLE_CONFIG_ADV_STEP_IDLE,
    BLE_CONFIG_ADV_STEP_SET_ADV_DATAS,
    BLE_CONFIG_ADV_STEP_SET_SCAN_RSP_DATAS,
    BLE_CONFIG_ADV_STEP_SET_GAP_DEVICE_NAME,
    BLE_CONFIG_ADV_STEP_START,
    BLE_CONFIG_ADV_STEP_END,
} ble_config_adv_step_t;

/* Private defines------------------------------------------------------------*/

/* Private types, structures, unions -----------------------------------------*/

typedef struct {
    // General
    ble_state_t         state;
    char                device_name[BLE_GAP_MAX_LOCAL_NAME_LENGTH + 1];
    char                device_name_length;
    uint8_t             random_address[BLE_CONFIG_DATA_RANDOM_ADDRESS_LEN];
    uint8_t             nb_of_profile;
    ble_profile_info_t *profile[2];
    uint16_t            profiles;
    uint8_t             ble_ready;

    // Init
    ble_init_step_t init_step;

    // Advertising configuration
    ble_config_adv_step_t adv_step;
    uint8_t               adv_enable;
    uint8_t               enabling_advertising;
    uint8_t               disabling_advertising;

    // HCI
    ble_cmd_data_t cmd_data;

    // GAP
    uint16_t         gap_service_handle;
    uint16_t         gap_device_name_characteristic_handle;
    uint16_t         gap_appearance_characteristic_handle;
    uint8_t          advertising_enabled;
    ble_connection_t connection;
    uint16_t         pairing_code;
    uint8_t          pairing_in_progress;

    // PAIRING
    uint8_t clear_pairing;

    // Name
    uint8_t name_changed;

} ble_ledger_data_t;

#ifdef HAVE_PRINTF
#define DEBUG PRINTF
// #define DEBUG(...)
#else  // !HAVE_PRINTF
#define DEBUG(...)
#endif  // !HAVE_PRINTF

/* Private macros-------------------------------------------------------------*/

/* Private functions prototypes ----------------------------------------------*/
// Init
static void get_device_name(void);
static void init_mngr(uint8_t *hci_buffer, uint16_t length);

// Advertising
static void configure_advertising_mngr(uint16_t opcode);
static void advertising_enable(uint8_t enable);
static void start_advertising(void);

#ifdef HAVE_INAPP_BLE_PAIRING
// Pairing UX
static void ask_user_pairing_numeric_comparison(uint32_t code);
static void rsp_user_pairing_numeric_comparison(unsigned int status);
static void ask_user_pairing_passkey(void);
static void rsp_user_pairing_passkey(unsigned int status);
static void end_pairing_ux(uint8_t pairing_ok);
#endif  // HAVE_INAPP_BLE_PAIRING

// HCI
static void hci_evt_cmd_complete(uint8_t *buffer, uint16_t length);
static void hci_evt_le_meta_evt(uint8_t *buffer, uint16_t length);
static void hci_evt_vendor(uint8_t *buffer, uint16_t length);

static uint32_t send_hci_packet(uint32_t timeout_ms);

/* Exported variables --------------------------------------------------------*/
uint8_t BLE_LEDGER_apdu_buffer[OS_IO_BUFFER_SIZE + 1];

/* Private variables ---------------------------------------------------------*/
static ble_ledger_data_t ble_ledger_data;

/* Private functions ---------------------------------------------------------*/
static void get_device_name(void)
{
    memset(ble_ledger_data.device_name, 0, sizeof(ble_ledger_data.device_name));
    ble_ledger_data.device_name_length = os_setting_get(OS_SETTING_DEVICENAME,
                                                        (uint8_t *) ble_ledger_data.device_name,
                                                        sizeof(ble_ledger_data.device_name) - 1);
}

static void init_mngr(uint8_t *hci_buffer, uint16_t length)
{
    if (ble_ledger_data.init_step == BLE_INIT_STEP_IDLE) {
        DEBUG("INIT START\n");
    }
    else if (hci_buffer) {
        uint16_t opcode = U2LE(hci_buffer, 1);
        if ((ble_ledger_data.cmd_data.hci_cmd_opcode != 0xFFFF)
            && (opcode != ble_ledger_data.cmd_data.hci_cmd_opcode)) {
            // Unexpected event => BLE_TODO
            return;
        }
        if ((length >= 6) && (ble_ledger_data.init_step == BLE_INIT_STEP_GAP_INIT)) {
            ble_ledger_data.gap_service_handle                    = U2LE(hci_buffer, 4);
            ble_ledger_data.gap_device_name_characteristic_handle = U2LE(hci_buffer, 6);
            ble_ledger_data.gap_appearance_characteristic_handle  = U2LE(hci_buffer, 8);
            DEBUG("GAP service handle          : %04X\n", ble_ledger_data.gap_service_handle);
            DEBUG("GAP device name char handle : %04X\n",
                  ble_ledger_data.gap_device_name_characteristic_handle);
            DEBUG("GAP appearance char handle  : %04X\n",
                  ble_ledger_data.gap_appearance_characteristic_handle);
        }
        else if (ble_ledger_data.init_step == BLE_INIT_STEP_CONFIGURE_ADVERTISING) {
            ble_ledger_data.adv_enable = !os_setting_get(OS_SETTING_PLANEMODE, NULL, 0);
            configure_advertising_mngr(opcode);
            if (ble_ledger_data.adv_step != BLE_CONFIG_ADV_STEP_END) {
                return;
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
                                       sizeof(ble_ledger_data.device_name) - 1);
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
            profile_info                     = ble_ledger_data.profile[0];
            if (ble_ledger_data.init_step == BLE_INIT_STEP_PROFILE_INIT) {
                if (profile_info->init) {
                    ((ble_profile_init_t) PIC(profile_info->init))(&ble_ledger_data.cmd_data,
                                                                   profile_info->cookie);
                }
                ble_ledger_data.init_step++;
            }
            if (profile_info->create_db) {
                if (((ble_profile_create_db_t) PIC(profile_info->create_db))(
                        hci_buffer, length, profile_info->cookie)
                    != BLE_PROFILE_STATUS_OK_PROCEDURE_END) {
                    ble_ledger_data.init_step--;
                    send_hci_packet(0);
                }
                else {
                    init_mngr(hci_buffer, length);
                }
            }
            else {
                init_mngr(hci_buffer, length);
            }
            break;
        }

        case BLE_INIT_STEP_SET_TX_POWER_LEVEL:
            ble_aci_hal_forge_cmd_set_tx_power_level(&ble_ledger_data.cmd_data,
                                                     1,  // High power
#ifdef TARGET_STAX
                                                     0x19);  // 0 dBm
#else                                                        // !TARGET_STAX
                                                     0x07);  // -14.1 dBm
#endif                                                       // !TARGET_STAX
            send_hci_packet(0);
            break;

        case BLE_INIT_STEP_CONFIGURE_ADVERTISING:
            ble_ledger_data.cmd_data.hci_cmd_opcode = 0xFFFF;
            ble_ledger_data.adv_step                = BLE_CONFIG_ADV_STEP_IDLE;
            ble_ledger_data.adv_enable = !os_setting_get(OS_SETTING_PLANEMODE, NULL, 0);
            configure_advertising_mngr(0);
            break;

        case BLE_INIT_STEP_END:
            DEBUG("INIT END\n");
            if (ble_ledger_data.clear_pairing == 0xC1) {
                ble_ledger_data.clear_pairing = 0;
                ble_aci_gap_forge_cmd_clear_security_db(&ble_ledger_data.cmd_data);
                send_hci_packet(0);
            }
            ble_ledger_data.ble_ready = 1;
            ble_ledger_data.state     = BLE_STATE_INITIALIZED;
            break;

        default:
            break;
    }
}

static void configure_advertising_mngr(uint16_t opcode)
{
    uint8_t buffer[31];
    uint8_t index = 0;

    if ((ble_ledger_data.cmd_data.hci_cmd_opcode != 0xFFFF)
        && (opcode != ble_ledger_data.cmd_data.hci_cmd_opcode)) {
        // Unexpected event => BLE_TODO
        return;
    }

    if (ble_ledger_data.adv_step == BLE_CONFIG_ADV_STEP_IDLE) {
        ble_ledger_data.connection.connection_handle = 0xFFFF;
        ble_ledger_data.advertising_enabled          = 0;
        DEBUG("CONFIGURE ADVERTISING START\n");
    }
    else if (ble_ledger_data.adv_step == (BLE_CONFIG_ADV_STEP_END - 1)) {
        ble_ledger_data.advertising_enabled = 1;
    }

    ble_ledger_data.adv_step++;
    if ((ble_ledger_data.adv_step == (BLE_CONFIG_ADV_STEP_END - 1))
        && (!ble_ledger_data.adv_enable)) {
        ble_ledger_data.adv_step++;
    }

    switch (ble_ledger_data.adv_step) {
        case BLE_CONFIG_ADV_STEP_SET_ADV_DATAS:
            // Flags
            buffer[index++] = 2;
            buffer[index++] = BLE_AD_TYPE_FLAGS;
            buffer[index++] = BLE_AD_TYPE_FLAG_BIT_BR_EDR_NOT_SUPPORTED
                              | BLE_AD_TYPE_FLAG_BIT_LE_GENERAL_DISCOVERABLE_MODE;

            // Complete Local Name
            get_device_name();
            buffer[index++] = ble_ledger_data.device_name_length + 1;
            buffer[index++] = BLE_AD_TYPE_COMPLETE_LOCAL_NAME;
            memcpy(&buffer[index], ble_ledger_data.device_name, ble_ledger_data.device_name_length);
            index += ble_ledger_data.device_name_length;

            ble_aci_gap_forge_cmd_update_adv_data(&ble_ledger_data.cmd_data, index, buffer);
            send_hci_packet(0);
            break;

        case BLE_CONFIG_ADV_STEP_SET_SCAN_RSP_DATAS: {
            ble_profile_info_t *profile_info = NULL;
            profile_info                     = ble_ledger_data.profile[0];
            // BLE_TODO
            // Incomplete List of 128-bit Service UUIDs
            buffer[index++] = 16 + 1;
            buffer[index++] = BLE_AD_AD_TYPE_128_BIT_SERV_UUID;
            memcpy(&buffer[index], (uint8_t *) PIC(profile_info->service_uuid.value), 16);
            index += 16;

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

        case BLE_CONFIG_ADV_STEP_SET_GAP_DEVICE_NAME:
            ble_aci_gatt_forge_cmd_update_char_value(
                &ble_ledger_data.cmd_data,
                ble_ledger_data.gap_service_handle,
                ble_ledger_data.gap_device_name_characteristic_handle,
                0,
                ble_ledger_data.device_name_length,
                (uint8_t *) ble_ledger_data.device_name);
            send_hci_packet(0);
            break;

        case BLE_CONFIG_ADV_STEP_START:
            advertising_enable(1);
            break;

        default:
            DEBUG("CONFIGURE ADVERTISING END\n");
            if (ble_ledger_data.state == BLE_STATE_CONFIGURE_ADVERTISING) {
                ble_ledger_data.state = BLE_STATE_INITIALIZED;
            }
            break;
    }
}

static void advertising_enable(uint8_t enable)
{
    if (enable) {
        uint8_t buffer[31];

        get_device_name();
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
    }
    else {
        ble_aci_gap_forge_cmd_set_non_discoverable(&ble_ledger_data.cmd_data);
    }
    send_hci_packet(0);
}

static void start_advertising(void)
{
    if (ble_ledger_data.name_changed) {
        ble_ledger_data.name_changed = 0;
        ble_ledger_data.state        = BLE_STATE_CONFIGURE_ADVERTISING;
        ble_ledger_data.adv_step     = BLE_CONFIG_ADV_STEP_IDLE;
    }
    else {
        ble_ledger_data.state    = BLE_STATE_CONFIGURE_ADVERTISING;
        ble_ledger_data.adv_step = BLE_CONFIG_ADV_STEP_START - 1;
    }
    ble_ledger_data.cmd_data.hci_cmd_opcode = 0xFFFF;
    ble_ledger_data.adv_enable              = !os_setting_get(OS_SETTING_PLANEMODE, NULL, 0);
    configure_advertising_mngr(0);
}

#ifdef HAVE_INAPP_BLE_PAIRING
static void ask_user_pairing_numeric_comparison(uint32_t code)
{
    bolos_ux_params_t ux_params;

    ux_params.u.pairing_request.type             = BOLOS_UX_ASYNCHMODAL_PAIRING_REQUEST_NUMCOMP;
    ux_params.u.pairing_request.pairing_info_len = 6;
    ux_params.ux_id                              = BOLOS_UX_ASYNCHMODAL_PAIRING_REQUEST;
    ux_params.len                                = sizeof(ux_params.u.pairing_request);
    ble_ledger_data.pairing_in_progress          = 1;

    SPRINTF(ux_params.u.pairing_request.pairing_info, "%06d", (unsigned int) code);

    os_io_ux_cmd_ble_pairing_request(&ux_params);
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
    bolos_ux_params_t ux_params;

    ux_params.u.pairing_request.type             = BOLOS_UX_ASYNCHMODAL_PAIRING_REQUEST_PASSKEY;
    ux_params.u.pairing_request.pairing_info_len = 6;
    ux_params.ux_id                              = BOLOS_UX_ASYNCHMODAL_PAIRING_REQUEST;
    ux_params.len                                = sizeof(ux_params.u.pairing_request);
    ble_ledger_data.pairing_in_progress          = 2;
    ble_ledger_data.pairing_code                 = cx_rng_u32_range_func(0, 1000000, cx_rng_u32);

    SPRINTF(ux_params.u.pairing_request.pairing_info, "%06d", ble_ledger_data.pairing_code);

    os_io_ux_cmd_ble_pairing_request(&ux_params);
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
    bolos_ux_params_t ux_params;

    DEBUG("end_pairing_ux : %d (%d)\n", pairing_ok, ble_ledger_data.pairing_in_progress);
    if (ble_ledger_data.pairing_in_progress) {
        ux_params.ux_id                       = BOLOS_UX_ASYNCHMODAL_PAIRING_STATUS;
        ux_params.len                         = sizeof(ux_params.u.pairing_status);
        ux_params.u.pairing_status.pairing_ok = pairing_ok;
        os_io_ux_cmd_ble_pairing_status(&ux_params);
    }
}
#endif  // HAVE_INAPP_BLE_PAIRING

static void hci_evt_cmd_complete(uint8_t *buffer, uint16_t length)
{
    if (length < 3) {
        return;
    }

    uint16_t opcode = U2LE(buffer, 1);

    if (ble_ledger_data.state == BLE_STATE_INITIALIZING) {
        init_mngr(buffer, length);
    }
    else if (ble_ledger_data.state == BLE_STATE_CONFIGURE_ADVERTISING) {
        configure_advertising_mngr(opcode);
    }
    else if (opcode == ACI_GATT_WRITE_RESP_CMD_CODE) {
        uint8_t             status       = BLE_PROFILE_STATUS_OK;
        ble_profile_info_t *profile_info = NULL;
        profile_info                     = ble_ledger_data.profile[0];
        if (profile_info->write_rsp_ack) {
            status = ((ble_profile_write_rsp_ack_t) PIC(profile_info->write_rsp_ack))(
                profile_info->cookie);
            if (status == BLE_PROFILE_STATUS_OK_AND_SEND_PACKET) {
                send_hci_packet(0);
            }
        }
    }
    else if (opcode == ACI_GATT_UPDATE_CHAR_VALUE_CMD_CODE) {
        uint8_t             status       = BLE_PROFILE_STATUS_OK;
        ble_profile_info_t *profile_info = NULL;
        profile_info                     = ble_ledger_data.profile[0];
        if (profile_info->update_char_val_ack) {
            status = ((ble_profile_update_char_value_ack_t) PIC(profile_info->update_char_val_ack))(
                profile_info->cookie);
            if (status == BLE_PROFILE_STATUS_OK_AND_SEND_PACKET) {
                send_hci_packet(0);
            }
        }
    }
    else if ((opcode == ACI_GAP_SET_NON_DISCOVERABLE_CMD_CODE)
             || (opcode == ACI_GAP_SET_DISCOVERABLE_CMD_CODE)) {
        DEBUG("HCI_LE_SET_ADVERTISE_ENABLE %04X %d %d\n",
              ble_ledger_data.connection.connection_handle,
              ble_ledger_data.disabling_advertising,
              ble_ledger_data.enabling_advertising);
        if (ble_ledger_data.connection.connection_handle != 0xFFFF) {
            if (ble_ledger_data.disabling_advertising) {
                // Connected & ordered to disable ble, force disconnection
#ifdef HAVE_INAPP_BLE_PAIRING
                end_pairing_ux(BOLOS_UX_ASYNCHMODAL_PAIRING_STATUS_FAILED);
#endif                              // HAVE_INAPP_BLE_PAIRING
                BLE_LEDGER_init();  // BLE_TODO
            }
        }
        else if (ble_ledger_data.disabling_advertising) {
            ble_ledger_data.advertising_enabled = 0;
            if (ble_ledger_data.name_changed) {
                start_advertising();
            }
        }
        else if (ble_ledger_data.enabling_advertising) {
            ble_ledger_data.advertising_enabled = 1;
        }
        else {
            ble_ledger_data.advertising_enabled = 1;
        }
        ble_ledger_data.disabling_advertising = 0;
        ble_ledger_data.enabling_advertising  = 0;
    }
    else if (opcode == ACI_GAP_NUMERIC_COMPARISON_VALUE_CONFIRM_YESNO_CMD_CODE) {
        DEBUG("ACI_GAP_NUMERIC_COMPARISON_VALUE_CONFIRM_YESNO\n");
    }
    else if (opcode == ACI_GAP_CLEAR_SECURITY_DB_CMD_CODE) {
        DEBUG("ACI_GAP_CLEAR_SECURITY_DB\n");
    }
    else if (opcode == ACI_GATT_CONFIRM_INDICATION_CMD_CODE) {
        DEBUG("ACI_GATT_CONFIRM_INDICATION\n");
    }
    else {
        DEBUG("HCI EVT CMD COMPLETE 0x%04X\n", opcode);
    }
}

static void hci_evt_le_meta_evt(uint8_t *buffer, uint16_t length)
{
    if (!length) {
        return;
    }

    ble_profile_info_t *profile_info = NULL;

    switch (buffer[0]) {
        case HCI_LE_CONNECTION_COMPLETE_SUBEVT_CODE:
            ble_ledger_data.connection.connection_handle   = U2LE(buffer, 2);
            ble_ledger_data.connection.role_slave          = buffer[4];
            ble_ledger_data.connection.peer_address_random = buffer[5];
            memcpy(ble_ledger_data.connection.peer_address, &buffer[6], 6);
            ble_ledger_data.connection.conn_interval         = U2LE(buffer, 12);
            ble_ledger_data.connection.conn_latency          = U2LE(buffer, 14);
            ble_ledger_data.connection.supervision_timeout   = U2LE(buffer, 16);
            ble_ledger_data.connection.master_clock_accuracy = buffer[18];
            ble_ledger_data.connection.encrypted             = 0;
            DEBUG("LE CONNECTION COMPLETE %04X - %04X- %04X- %04X\n",
                  ble_ledger_data.connection.connection_handle,
                  ble_ledger_data.connection.conn_interval,
                  ble_ledger_data.connection.conn_latency,
                  ble_ledger_data.connection.supervision_timeout);
            ble_ledger_data.advertising_enabled = 0;

            profile_info = ble_ledger_data.profile[0];
            if (profile_info->connection_evt) {
                ((ble_profile_connection_evt_t) PIC(profile_info->connection_evt))(
                    &ble_ledger_data.connection, profile_info->cookie);
            }
            break;

        case HCI_LE_CONNECTION_UPDATE_COMPLETE_SUBEVT_CODE:
            ble_ledger_data.connection.connection_handle   = U2LE(buffer, 2);
            ble_ledger_data.connection.conn_interval       = U2LE(buffer, 4);
            ble_ledger_data.connection.conn_latency        = U2LE(buffer, 6);
            ble_ledger_data.connection.supervision_timeout = U2LE(buffer, 8);
            DEBUG("LE CONNECTION UPDATE %04X - %04X- %04X- %04X\n",
                  ble_ledger_data.connection.connection_handle,
                  ble_ledger_data.connection.conn_interval,
                  ble_ledger_data.connection.conn_latency,
                  ble_ledger_data.connection.supervision_timeout);

            profile_info = ble_ledger_data.profile[0];
            if (profile_info->connection_update_evt) {
                ((ble_profile_connection_update_evt_t) PIC(profile_info->connection_update_evt))(
                    &ble_ledger_data.connection, profile_info->cookie);
            }
            break;

        case HCI_LE_DATA_LENGTH_CHANGE_SUBEVT_CODE:
            if (U2LE(buffer, 1) == ble_ledger_data.connection.connection_handle) {
                ble_ledger_data.connection.max_tx_octets = U2LE(buffer, 3);
                ble_ledger_data.connection.max_tx_time   = U2LE(buffer, 5);
                ble_ledger_data.connection.max_rx_octets = U2LE(buffer, 7);
                ble_ledger_data.connection.max_rx_time   = U2LE(buffer, 9);
                DEBUG("LE DATA LENGTH CHANGE %04X - %04X- %04X- %04X\n",
                      ble_ledger_data.connection.max_tx_octets,
                      ble_ledger_data.connection.max_tx_time,
                      ble_ledger_data.connection.max_rx_octets,
                      ble_ledger_data.connection.max_rx_time);
            }
            break;

        case HCI_LE_PHY_UPDATE_COMPLETE_SUBEVT_CODE:
            if (U2LE(buffer, 2) == ble_ledger_data.connection.connection_handle) {
                ble_ledger_data.connection.tx_phy = buffer[4];
                ble_ledger_data.connection.rx_phy = buffer[5];
                DEBUG("LE PHY UPDATE %02X - %02X\n",
                      ble_ledger_data.connection.tx_phy,
                      ble_ledger_data.connection.rx_phy);
            }
            break;

        default:
            DEBUG("HCI LE META 0x%02X\n", buffer[0]);
            break;
    }
}

static void hci_evt_vendor(uint8_t *buffer, uint16_t length)
{
    if (length < 4) {
        return;
    }

    uint16_t opcode = U2LE(buffer, 0);

    if (U2LE(buffer, 2) != ble_ledger_data.connection.connection_handle) {
        return;
    }

    switch (opcode) {
#ifdef HAVE_INAPP_BLE_PAIRING
        case ACI_GAP_PAIRING_COMPLETE_VSEVT_CODE:
            DEBUG("PAIRING");
            switch (buffer[4]) {
                case SMP_PAIRING_STATUS_SUCCESS:
                    DEBUG(" SUCCESS\n");
                    end_pairing_ux(BOLOS_UX_ASYNCHMODAL_PAIRING_STATUS_SUCCESS);
                    break;

                case SMP_PAIRING_STATUS_SMP_TIMEOUT:
                    DEBUG(" TIMEOUT\n");
                    end_pairing_ux(BOLOS_UX_ASYNCHMODAL_PAIRING_STATUS_TIMEOUT);
                    break;

                case SMP_PAIRING_STATUS_PAIRING_FAILED:
                    DEBUG(" FAILED : %02X\n", buffer[5]);
                    if (buffer[5] == 0x08) {  // UNSPECIFIED_REASON
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
            DEBUG("PASSKEY REQ\n");
            ask_user_pairing_passkey();
            break;

        case ACI_GAP_NUMERIC_COMPARISON_VALUE_VSEVT_CODE:
            DEBUG("NUMERIC COMP : %d\n", U4LE(buffer, 4));
            ask_user_pairing_numeric_comparison(U4LE(buffer, 4));
            break;
#endif  // HAVE_INAPP_BLE_PAIRING

        case ACI_L2CAP_CONNECTION_UPDATE_RESP_VSEVT_CODE: {
            DEBUG("CONNECTION UPDATE RESP %d\n", buffer[4]);
            break;
        }

        case ACI_GATT_ATTRIBUTE_MODIFIED_VSEVT_CODE:
        case ACI_GATT_WRITE_PERMIT_REQ_VSEVT_CODE: {
            uint16_t            att_handle   = U2LE(buffer, 4);
            ble_profile_info_t *profile_info = NULL;
            profile_info                     = ble_ledger_data.profile[0];
            if ((profile_info->handle_in_range)
                && (((ble_profile_handle_in_range_t) PIC(profile_info->handle_in_range))(
                    att_handle, profile_info->cookie))) {
                uint8_t status = BLE_PROFILE_STATUS_OK;
                if (opcode == ACI_GATT_ATTRIBUTE_MODIFIED_VSEVT_CODE) {
                    if (profile_info->att_modified) {
                        status = ((ble_profile_att_modified_t) PIC(profile_info->att_modified))(
                            buffer, length, profile_info->cookie);
                    }
                }
                else if (opcode == ACI_GATT_WRITE_PERMIT_REQ_VSEVT_CODE) {
                    if (profile_info->write_permit_req) {
                        status = ((ble_profile_write_permit_req_t) PIC(
                            profile_info->write_permit_req))(buffer, length, profile_info->cookie);
                    }
                }
                if (status == BLE_PROFILE_STATUS_OK_AND_SEND_PACKET) {
                    send_hci_packet(0);
                }
            }
            else if (opcode == ACI_GATT_ATTRIBUTE_MODIFIED_VSEVT_CODE) {
                DEBUG("ATT MODIFIED %04X %d bytes at offset %d\n",
                      att_handle,
                      U2LE(buffer, 8),
                      U2LE(buffer, 6));
            }
            break;
        }

        case ACI_ATT_EXCHANGE_MTU_RESP_VSEVT_CODE:
            DEBUG("MTU : %d\n", U2LE(buffer, 4));

            ble_profile_info_t *profile_info = NULL;
            profile_info                     = ble_ledger_data.profile[0];
            if (profile_info->mtu_changed) {
                uint8_t status = BLE_PROFILE_STATUS_OK;
                status         = ((ble_profile_mtu_changed_t) PIC(profile_info->mtu_changed))(
                    U2LE(buffer, 4), profile_info->cookie);
                if (status == BLE_PROFILE_STATUS_OK_AND_SEND_PACKET) {
                    send_hci_packet(0);
                }
            }
            break;

        case ACI_GATT_INDICATION_VSEVT_CODE:
            DEBUG("INDICATION EVT\n");
            ble_aci_gatt_forge_cmd_confirm_indication(&ble_ledger_data.cmd_data,
                                                      ble_ledger_data.connection.connection_handle);
            send_hci_packet(0);
            break;

        case ACI_GATT_PROC_COMPLETE_VSEVT_CODE:
            DEBUG("PROCEDURE COMPLETE\n");
            break;

        case ACI_GATT_PROC_TIMEOUT_VSEVT_CODE:
            DEBUG("PROCEDURE TIMEOUT\n");
#ifdef HAVE_INAPP_BLE_PAIRING
            end_pairing_ux(BOLOS_UX_ASYNCHMODAL_PAIRING_STATUS_FAILED);
#endif                          // HAVE_INAPP_BLE_PAIRING
            BLE_LEDGER_init();  // BLE_TODO
            break;

        default:
            DEBUG("HCI VENDOR 0x%04X\n", opcode);
            break;
    }
}

static uint32_t send_hci_packet(uint32_t timeout_ms)
{
    uint8_t hdr[3];

    UNUSED(timeout_ms);

    hdr[0] = SEPROXYHAL_TAG_BLE_SEND;
    U2BE_ENCODE(hdr, 1, ble_ledger_data.cmd_data.hci_cmd_buffer_length);
    os_io_tx_cmd(OS_IO_PACKET_TYPE_SEPH, hdr, 3, NULL);
    os_io_tx_cmd(OS_IO_PACKET_TYPE_SEPH,
                 ble_ledger_data.cmd_data.hci_cmd_buffer,
                 ble_ledger_data.cmd_data.hci_cmd_buffer_length,
                 NULL);

    return 0;
}

/* Exported functions --------------------------------------------------------*/
void BLE_LEDGER_init(void)
{
    if (ble_ledger_data.clear_pairing == 0xC1) {
        memset(&ble_ledger_data, 0, sizeof(ble_ledger_data));
        ble_ledger_data.clear_pairing = 0xC1;
    }
    else {
        memset(&ble_ledger_data, 0, sizeof(ble_ledger_data));
    }
}

void BLE_LEDGER_start(uint16_t profile_mask)
{
    DEBUG("BLE_LEDGER_start\n");
    if (!profile_mask) {
        advertising_enable(0);
        ble_ledger_data.profiles = profile_mask;
    }
    else if (ble_ledger_data.profiles != profile_mask) {
        LEDGER_BLE_get_mac_address(ble_ledger_data.random_address);
        ble_ledger_data.cmd_data.hci_cmd_opcode = 0xFFFF;
        ble_ledger_data.state                   = BLE_STATE_INITIALIZING;
        ble_ledger_data.init_step               = BLE_INIT_STEP_IDLE;
        ble_ledger_data.nb_of_profile           = 0;

        if (profile_mask & BLE_LEDGER_PROFILE_APDU) {
            DEBUG("APDU ");
            ble_ledger_data.profile[ble_ledger_data.nb_of_profile++]
                = (ble_profile_info_t *) PIC(&BLE_LEDGER_PROFILE_apdu_info);
        }
#ifdef HAVE_IO_U2F
        else if (profile_mask & BLE_LEDGER_PROFILE_U2F) {
            DEBUG("U2F ");
            // ble_ledger_data.profile[ble_ledger_data.nb_of_profile++] =
            // (ble_profile_info_t*)PIC(&BLE_LEDGER_PROFILE_u2f_info);
        }
        DEBUG("\n");
#endif  // HAVE_IO_U2F
        ble_ledger_data.profiles = profile_mask;
        init_mngr(NULL, 0);
    }
}

void BLE_LEDGER_enable_advertising(uint8_t enable)
{
    if ((ble_ledger_data.name_changed) && (ble_ledger_data.ble_ready) && (!enable)
        && (ble_ledger_data.connection.connection_handle != 0xFFFF)) {
        ble_ledger_data.name_changed = 0;
    }
    else if (ble_ledger_data.ble_ready) {
        if (enable) {
            ble_ledger_data.enabling_advertising  = 1;
            ble_ledger_data.disabling_advertising = 0;
        }
        else {
            ble_ledger_data.enabling_advertising  = 0;
            ble_ledger_data.disabling_advertising = 1;
        }
        advertising_enable(enable);
    }
}

void BLE_LEDGER_reset_pairings(void)
{
    if (ble_ledger_data.ble_ready) {
        if (ble_ledger_data.connection.connection_handle != 0xFFFF) {
            // Connected => force disconnection before clearing
            ble_ledger_data.clear_pairing = 0xC1;
            BLE_LEDGER_init();
        }
        else {
            ble_aci_gap_forge_cmd_clear_security_db(&ble_ledger_data.cmd_data);
            send_hci_packet(0);
        }
    }
}

void BLE_LEDGER_accept_pairing(uint8_t status)
{
    if (ble_ledger_data.pairing_in_progress == 1) {
        rsp_user_pairing_numeric_comparison(status);
    }
    else if (ble_ledger_data.pairing_in_progress == 2) {
        rsp_user_pairing_passkey(status);
    }
}

void BLE_LEDGER_name_changed(void)
{
    ble_ledger_data.name_changed = 1;
    BLE_LEDGER_enable_advertising(0);
}

int BLE_LEDGER_rx_seph_evt(uint8_t *seph_buffer,
                           uint16_t seph_buffer_length,
                           uint8_t *apdu_buffer,
                           uint16_t apdu_buffer_max_length)
{
    uint32_t status = 0;

    if (seph_buffer_length < 5) {
        return -1;
    }

    if (seph_buffer[4] == HCI_EVENT_PKT_TYPE) {
        switch (seph_buffer[5]) {
            case HCI_DISCONNECTION_COMPLETE_EVT_CODE:
                if (seph_buffer_length < 10) {
                    status = -1;
                }
                else {
                    DEBUG("HCI DISCONNECTION COMPLETE code %02X\n", seph_buffer[9]);
                    ble_ledger_data.connection.connection_handle = 0xFFFF;
                    ble_ledger_data.advertising_enabled          = 0;
                    ble_ledger_data.connection.encrypted         = 0;
#ifdef HAVE_INAPP_BLE_PAIRING
                    end_pairing_ux(BOLOS_UX_ASYNCHMODAL_PAIRING_STATUS_FAILED);
#endif  // HAVE_INAPP_BLE_PAIRING
                    start_advertising();
                }
                break;

            case HCI_ENCRYPTION_CHANGE_EVT_CODE:
                if (seph_buffer_length < 10) {
                    status = -1;
                }
                else if (U2LE(seph_buffer, 8) == ble_ledger_data.connection.connection_handle) {
                    if (seph_buffer[10]) {
                        DEBUG("Link encrypted\n");
                        ble_ledger_data.connection.encrypted = 1;
                    }
                    else {
                        DEBUG("Link not encrypted\n");
                        ble_ledger_data.connection.encrypted = 0;
                    }
                    ble_profile_info_t *profile_info = NULL;
                    profile_info                     = ble_ledger_data.profile[0];
                    if (profile_info->encryption_changed) {
                        ((ble_profile_encryption_changed_t) PIC(profile_info->encryption_changed))(
                            ble_ledger_data.connection.encrypted, profile_info->cookie);
                    }
                }
                else {
                    DEBUG("HCI ENCRYPTION CHANGE EVT %d on connection handle \n",
                          seph_buffer[10],
                          U2LE(seph_buffer, 8));
                }
                break;

            case HCI_COMMAND_COMPLETE_EVT_CODE:
                if (seph_buffer_length < 8) {
                    status = -1;
                }
                else {
                    hci_evt_cmd_complete(&seph_buffer[7], seph_buffer[6]);
                }
                break;

            case HCI_COMMAND_STATUS_EVT_CODE:
                DEBUG("HCI COMMAND_STATUS %d - num %d - op %04X\n",
                      seph_buffer[7],
                      seph_buffer[8],
                      U2LE(seph_buffer, 9));
                break;

            case HCI_ENCRYPTION_KEY_REFRESH_COMPLETE_EVT_CODE:
                DEBUG("HCI KEY_REFRESH_COMPLETE\n");
                break;

            case HCI_LE_META_EVT_CODE:
                if (seph_buffer_length < 8) {
                    status = -1;
                }
                else {
                    hci_evt_le_meta_evt(&seph_buffer[7], seph_buffer[6]);
                }
                break;

            case HCI_VENDOR_SPECIFIC_DEBUG_EVT_CODE:
                if (seph_buffer_length < 8) {
                    status = -1;
                }
                else {
                    hci_evt_vendor(&seph_buffer[7], seph_buffer[6]);
                    status = BLE_LEDGER_data_ready(apdu_buffer, apdu_buffer_max_length);
                }
                break;

            default:
                break;
        }
    }

    return status;
}

uint32_t BLE_LEDGER_send(const uint8_t *packet, uint16_t packet_length, uint32_t timeout_ms)
{
    uint32_t            status       = SWO_SUCCESS;
    uint8_t             ble_status   = BLE_PROFILE_STATUS_OK;
    ble_profile_info_t *profile_info = NULL;

    profile_info = ble_ledger_data.profile[0];

    if (profile_info->send_packet) {
        ble_status = ((ble_profile_send_packet_t) PIC(profile_info->send_packet))(
            packet, packet_length, profile_info->cookie);
    }

    if (ble_status == BLE_PROFILE_STATUS_OK_AND_SEND_PACKET) {
        status = send_hci_packet(timeout_ms);
    }

    return status;
}

int32_t BLE_LEDGER_data_ready(uint8_t *buffer, uint16_t max_length)
{
    uint8_t index  = 0;
    int32_t status = 0;

    ble_profile_info_t *profile_info = NULL;
    for (index = 0; index < ble_ledger_data.nb_of_profile; index++) {
        profile_info = ble_ledger_data.profile[index];
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

void BLE_LEDGER_setting(uint32_t profile_id, uint32_t setting_id, uint8_t *buffer, uint16_t length)
{
    uint8_t index = 0;

    ble_profile_info_t *profile_info = NULL;
    for (index = 0; index < ble_ledger_data.nb_of_profile; index++) {
        profile_info = ble_ledger_data.profile[index];
        if ((profile_info->type == profile_id) && (profile_info->setting)) {
            ((ble_profile_setting_t) PIC(profile_info->setting))(
                setting_id, buffer, length, profile_info->cookie);
        }
    }
}