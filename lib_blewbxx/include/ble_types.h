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

#define BLE_UUID_SIZE (16U)
/* Exported enumerations -----------------------------------------------------*/

/* HCI Error codes */
typedef enum ble_hci_error_codes_e {
    HCI_SUCCESS_ERR_CODE = 0x00,
} ble_hci_error_codes_t;

/* HCI Event codes */
typedef enum ble_hci_evt_codes_e {
    HCI_DISCONNECTION_COMPLETE_EVT_CODE          = 0x05,
    HCI_ENCRYPTION_CHANGE_EVT_CODE               = 0x08,
    HCI_COMMAND_COMPLETE_EVT_CODE                = 0x0e,
    HCI_COMMAND_STATUS_EVT_CODE                  = 0x0f,
    HCI_ENCRYPTION_KEY_REFRESH_COMPLETE_EVT_CODE = 0x30,
    HCI_LE_META_EVT_CODE                         = 0x3E,
    HCI_VENDOR_SPECIFIC_DEBUG_EVT_CODE           = 0xFF,
} ble_hci_evt_codes_t;

/* HCI Cmd codes */
typedef enum ble_hci_cmd_codes_e {
    HCI_DISCONNECT_CMD_CODE                = 0x0406,
    HCI_LE_SET_SCAN_RESPONSE_DATA_CMD_CODE = 0x2009,
    HCI_RESET_CMD_CODE                     = 0x0c03,
} ble_hci_cmd_codes_t;

/* HCI LE SubEvent codes */
typedef enum ble_hci_subevt_codes_e {
    HCI_LE_CONNECTION_COMPLETE_SUBEVT_CODE        = 0x01,
    HCI_LE_CONNECTION_UPDATE_COMPLETE_SUBEVT_CODE = 0x03,
    HCI_LE_DATA_LENGTH_CHANGE_SUBEVT_CODE         = 0x07,
    HCI_LE_PHY_UPDATE_COMPLETE_SUBEVT_CODE        = 0x0c,
} ble_hci_subevt_codes_t;

/* SMP pairing status */
typedef enum ble_smp_pairing_status_e {
    SMP_PAIRING_STATUS_SUCCESS        = 0x00U,
    SMP_PAIRING_STATUS_SMP_TIMEOUT    = 0x01U,
    SMP_PAIRING_STATUS_PAIRING_FAILED = 0x02U,
    SMP_PAIRING_STATUS_ENCRYPT_FAILED = 0x03U,
} ble_smp_pairing_status_t;

/* ACI vendor specific event codes */
typedef enum ble_aci_evt_codes_e {
    // GAP
    ACI_GAP_PAIRING_COMPLETE_VSEVT_CODE         = 0x0401,
    ACI_GAP_PASS_KEY_REQ_VSEVT_CODE             = 0x0402,
    ACI_GAP_NUMERIC_COMPARISON_VALUE_VSEVT_CODE = 0x0409,

    // L2CAP
    ACI_L2CAP_CONNECTION_UPDATE_RESP_VSEVT_CODE = 0x0800,

    // ATT
    ACI_ATT_EXCHANGE_MTU_RESP_VSEVT_CODE = 0x0c03,

    // GATT
    ACI_GATT_ATTRIBUTE_MODIFIED_VSEVT_CODE = 0x0c01,
    ACI_GATT_PROC_TIMEOUT_VSEVT_CODE       = 0x0c02,
    ACI_GATT_INDICATION_VSEVT_CODE         = 0x0c0e,
    ACI_GATT_PROC_COMPLETE_VSEVT_CODE      = 0x0c10,
    ACI_GATT_WRITE_PERMIT_REQ_VSEVT_CODE   = 0x0c13,
} ble_aci_evt_codes_t;

/* ACI vendor specific cmd codes */
typedef enum ble_aci_cmd_codes_e {
    ACI_HAL_WRITE_CONFIG_DATA_CMD_CODE                      = 0xfc0c,
    ACI_HAL_SET_TX_POWER_LEVEL_CMD_CODE                     = 0xfc0f,
    ACI_GAP_SET_NON_DISCOVERABLE_CMD_CODE                   = 0xfc81,
    ACI_GAP_SET_DISCOVERABLE_CMD_CODE                       = 0xfc83,
    ACI_GAP_SET_IO_CAPABILITY_CMD_CODE                      = 0xfc85,
    ACI_GAP_SET_AUTHENTICATION_REQUIREMENT_CMD_CODE         = 0xfc86,
    ACI_GAP_PASS_KEY_RESP_CMD_CODE                          = 0xfc88,
    ACI_GAP_INIT_CMD_CODE                                   = 0xfc8a,
    ACI_GAP_UPDATE_ADV_DATA_CMD_CODE                        = 0xfc8e,
    ACI_GAP_CLEAR_SECURITY_DB_CMD_CODE                      = 0xfc94,
    ACI_GAP_NUMERIC_COMPARISON_VALUE_CONFIRM_YESNO_CMD_CODE = 0xfca5,
    ACI_GATT_INIT_CMD_CODE                                  = 0xfd01,
    ACI_GATT_ADD_SERVICE_CMD_CODE                           = 0xfd02,
    ACI_GATT_ADD_CHAR_CMD_CODE                              = 0xfd04,
    ACI_GATT_UPDATE_CHAR_VALUE_CMD_CODE                     = 0xfd06,
    ACI_GATT_EXCHANGE_CONFIG_CMD_CODE                       = 0xfd0b,
    ACI_GATT_CONFIRM_INDICATION_CMD_CODE                    = 0xfd25,
    ACI_GATT_WRITE_RESP_CMD_CODE                            = 0xfd26,
    ACI_L2CAP_CONNECTION_PARAMETER_UPDATE_CMD_CODE          = 0xfd81,
} ble_aci_cmd_codes_t;

/* Adv/Scan AD types*/
typedef enum ble_ad_types_e {
    BLE_AD_TYPE_FLAGS                  = 0x01,
    BLE_AD_TYPE_COMPLETE_LOCAL_NAME    = 0x09,
    BLE_AD_AD_TYPE_128_BIT_SERV_UUID   = 0x06,
    BLE_AD_AD_TYPE_SLAVE_CONN_INTERVAL = 0x12,
} ble_ad_types_t;

/* AD type Flags*/
typedef enum ble_ad_type_flags_e {
    BLE_AD_TYPE_FLAG_BIT_LE_GENERAL_DISCOVERABLE_MODE = 0x02,
    BLE_AD_TYPE_FLAG_BIT_BR_EDR_NOT_SUPPORTED         = 0x04,
} ble_ad_type_flags_t;

/* Characteristic properties */
typedef enum ble_char_prop_e {
    BLE_CHAR_PROP_READ               = 0x02,
    BLE_CHAR_PROP_WRITE_WITHOUT_RESP = 0x04,
    BLE_CHAR_PROP_WRITE              = 0x08,
    BLE_CHAR_PROP_NOTIFY             = 0x10,
} ble_char_prop_t;

/* GATT notify events*/
typedef enum ble_gatt_notify_events_e {
    BLE_GATT_DONT_NOTIFY_EVENTS                      = 0x00,
    BLE_GATT_NOTIFY_ATTRIBUTE_WRITE                  = 0x01,
    BLE_GATT_NOTIFY_WRITE_REQ_AND_WAIT_FOR_APPL_RESP = 0x02,
    BLE_GATT_NOTIFY_READ_REQ_AND_WAIT_FOR_APPL_RESP  = 0x04,
} ble_gatt_notify_events_t;

/* UUID */
typedef enum ble_gatt_uuid_e {
    BLE_GATT_UUID_TYPE_16  = 0x01,
    BLE_GATT_UUID_TYPE_128 = 0x02,
} ble_gatt_uuid_e;

/* Exported types, structures, unions ----------------------------------------*/
typedef struct ble_cmd_set_auth_req_data_s {
    uint8_t  bonding_mode;
    uint8_t  mitm_mode;
    uint8_t  sc_support;
    uint8_t  key_press_notification_support;
    uint8_t  min_encryption_key_size;
    uint8_t  max_encryption_key_size;
    uint8_t  use_fixed_pin;
    uint32_t fixed_pin;
    uint8_t  identity_address_type;
} ble_cmd_set_auth_req_data_t;

typedef struct ble_cmd_set_discoverable_data_s {
    uint8_t        advertising_type;
    uint16_t       advertising_interval_min;
    uint16_t       advertising_interval_max;
    uint8_t        own_address_type;
    uint8_t        advertising_filter_policy;
    uint8_t        local_name_length;
    const uint8_t *local_name;
    uint8_t        service_uuid_length;
    const uint8_t *service_uuid_list;
    uint16_t       slave_conn_interval_min;
    uint16_t       slave_conn_interval_max;
} ble_cmd_set_discoverable_data_t;

typedef struct {
    uint16_t       service_handle;
    uint8_t        char_uuid_type;
    const uint8_t *char_uuid;
    uint16_t       char_value_length;
    uint8_t        char_properties;
    uint8_t        security_permissions;
    uint8_t        gatt_evt_mask;
    uint8_t        enc_key_size;
    uint8_t        is_variable;
} ble_cmd_add_char_data_t;

typedef struct ble_uuid_s {
    uint8_t type;  // ble_gatt_uuid_e
    uint8_t value[BLE_UUID_SIZE];
} ble_uuid_t;

typedef struct ble_hci_event_packet_s {
    uint8_t code;
    uint8_t length;
} ble_hci_event_packet_t;

/* Exported defines   --------------------------------------------------------*/
/* Configuration value */
#define BLE_CONFIG_DATA_RANDOM_ADDRESS_OFFSET 0x2e
#define BLE_CONFIG_DATA_RANDOM_ADDRESS_LEN    6U

/* HCI */
#define HCI_EVENT_PKT_TYPE 0x04

/* GAP init*/
#define BLE_GAP_PERIPHERAL_ROLE       0x01
#define BLE_GAP_PRIVACY_DISABLED      0x00
#define BLE_GAP_IO_CAP_DISPLAY_YES_NO 0x01

/* GAP set authentication */
#define BLE_GAP_BONDING_ENABLED                     0x01
#define BLE_GAP_MITM_PROTECTION_REQUIRED            0x01
#define BLE_GAP_KEYPRESS_NOT_SUPPORTED              0x00
#define BLE_GAP_MIN_ENCRYPTION_KEY_SIZE             8U
#define BLE_GAP_MAX_ENCRYPTION_KEY_SIZE             16U
#define BLE_GAP_USE_FIXED_PIN_FOR_PAIRING_FORBIDDEN 0x01
#define BLE_GAP_STATIC_RANDOM_ADDR                  0x01

/* GAP ADV/SCAN */
#define BLE_GAP_MAX_ADV_DATA_LEN      31U
#define BLE_GAP_MAX_LOCAL_NAME_LENGTH 20U
#define BLE_GAP_ADV_IND               0x00
#define BLE_GAP_RANDOM_ADDR_TYPE      0X01
#define BLE_GAP_NO_WHITE_LIST_USE     0X00

/* ATT/GATT */
#define BLE_ATT_DEFAULT_MTU                   23U
#define BLE_ATT_MAX_MTU_SIZE                  156U
#define BLE_GATT_UUID_TYPE_128                0x02
#define BLE_GATT_PRIMARY_SERVICE              0x01
#define BLE_GATT_MAX_SERVICE_RECORDS          9U
#define BLE_GATT_ATTR_PERMISSION_AUTHEN_WRITE 0x08
#define BLE_GATT_INVALID_HANDLE               0xFFFF

/* Exported macros------------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/

/* Exported functions prototypes--------------------------------------------- */
