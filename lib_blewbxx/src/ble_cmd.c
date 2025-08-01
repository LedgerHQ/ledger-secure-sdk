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
#include "os.h"
#include "ble_cmd.h"
#include <string.h>

/* Private enumerations ------------------------------------------------------*/

/* Private types, structures, unions -----------------------------------------*/

/* Private defines------------------------------------------------------------*/

/* Private macros-------------------------------------------------------------*/

/* Private functions prototypes ----------------------------------------------*/
static void start_packet_with_opcode(ble_cmd_data_t *cmd_data, uint16_t opcode);

/* Exported variables --------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private functions ---------------------------------------------------------*/
static void start_packet_with_opcode(ble_cmd_data_t *cmd_data, uint16_t opcode)
{
    cmd_data->hci_cmd_opcode        = opcode;
    cmd_data->hci_cmd_buffer_length = 2;
    U2BE_ENCODE(cmd_data->hci_cmd_buffer, 0, cmd_data->hci_cmd_opcode);
}

/* Exported functions --------------------------------------------------------*/
void ble_hci_forge_cmd_reset(ble_cmd_data_t *cmd_data)
{
    if (!cmd_data) {
        return;
    }

    start_packet_with_opcode(cmd_data, HCI_RESET_CMD_CODE);
}

void ble_hci_forge_cmd_disconnect(ble_cmd_data_t *cmd_data, uint16_t connection_handle)
{
    if (!cmd_data) {
        return;
    }

    start_packet_with_opcode(cmd_data, HCI_DISCONNECT_CMD_CODE);
    U2LE_ENCODE(cmd_data->hci_cmd_buffer, 2, connection_handle);
    cmd_data->hci_cmd_buffer_length = 4;
}

void ble_hci_forge_cmd_set_scan_response_data(ble_cmd_data_t *cmd_data,
                                              uint8_t         scan_response_data_length,
                                              const uint8_t  *scan_response_data)
{
    if ((!cmd_data) || (!scan_response_data)
        || ((uint16_t) (scan_response_data_length + 3)
            > (uint16_t) sizeof(cmd_data->hci_cmd_buffer))) {
        return;
    }

    memset(&cmd_data->hci_cmd_buffer, 0, sizeof(cmd_data->hci_cmd_buffer));
    start_packet_with_opcode(cmd_data, HCI_LE_SET_SCAN_RESPONSE_DATA_CMD_CODE);
    cmd_data->hci_cmd_buffer[2] = scan_response_data_length;
    memcpy(&cmd_data->hci_cmd_buffer[3], scan_response_data, scan_response_data_length);
    cmd_data->hci_cmd_buffer_length
        = MIN(BLE_GAP_MAX_ADV_DATA_LEN + 3, sizeof(cmd_data->hci_cmd_buffer));
}

void ble_aci_hal_forge_cmd_write_config_data(ble_cmd_data_t *cmd_data,
                                             uint8_t         offset,
                                             uint8_t         length,
                                             const uint8_t  *value)
{
    if ((!cmd_data) || (!value)
        || ((uint16_t) (length + 4) > (uint16_t) sizeof(cmd_data->hci_cmd_buffer))) {
        return;
    }

    start_packet_with_opcode(cmd_data, ACI_HAL_WRITE_CONFIG_DATA_CMD_CODE);
    cmd_data->hci_cmd_buffer[2] = offset;
    cmd_data->hci_cmd_buffer[3] = length;
    memcpy(&cmd_data->hci_cmd_buffer[4], value, length);
    cmd_data->hci_cmd_buffer_length = 4 + length;
}

void ble_aci_hal_forge_cmd_set_tx_power_level(ble_cmd_data_t *cmd_data,
                                              uint8_t         enable_high_power,
                                              uint8_t         pa_level)
{
    if (!cmd_data) {
        return;
    }

    start_packet_with_opcode(cmd_data, ACI_HAL_SET_TX_POWER_LEVEL_CMD_CODE);
    cmd_data->hci_cmd_buffer[2]     = enable_high_power;
    cmd_data->hci_cmd_buffer[3]     = pa_level;
    cmd_data->hci_cmd_buffer_length = 4;
}

void ble_aci_gap_forge_cmd_init(ble_cmd_data_t *cmd_data,
                                uint8_t         role,
                                uint8_t         privacy_enabled,
                                uint8_t         device_name_char_len)
{
    if (!cmd_data) {
        return;
    }

    start_packet_with_opcode(cmd_data, ACI_GAP_INIT_CMD_CODE);
    cmd_data->hci_cmd_buffer[2]     = role;
    cmd_data->hci_cmd_buffer[3]     = privacy_enabled;
    cmd_data->hci_cmd_buffer[4]     = device_name_char_len;
    cmd_data->hci_cmd_buffer_length = 5;
}

void ble_aci_gap_forge_cmd_set_io_capability(ble_cmd_data_t *cmd_data, uint8_t io_capability)
{
    if (!cmd_data) {
        return;
    }

    start_packet_with_opcode(cmd_data, ACI_GAP_SET_IO_CAPABILITY_CMD_CODE);
    cmd_data->hci_cmd_buffer[2]     = io_capability;
    cmd_data->hci_cmd_buffer_length = 3;
}

void ble_aci_gap_forge_cmd_set_authentication_requirement(ble_cmd_data_t              *cmd_data,
                                                          ble_cmd_set_auth_req_data_t *data)
{
    if ((!cmd_data) || (!data)) {
        return;
    }

    start_packet_with_opcode(cmd_data, ACI_GAP_SET_AUTHENTICATION_REQUIREMENT_CMD_CODE);
    cmd_data->hci_cmd_buffer[2] = data->bonding_mode;
    cmd_data->hci_cmd_buffer[3] = data->mitm_mode;
    cmd_data->hci_cmd_buffer[4] = data->sc_support;
    cmd_data->hci_cmd_buffer[5] = data->key_press_notification_support;
    cmd_data->hci_cmd_buffer[6] = data->min_encryption_key_size;
    cmd_data->hci_cmd_buffer[7] = data->max_encryption_key_size;
    cmd_data->hci_cmd_buffer[8] = data->use_fixed_pin;
    U4LE_ENCODE(cmd_data->hci_cmd_buffer, 9, data->fixed_pin);
    cmd_data->hci_cmd_buffer[13]    = data->identity_address_type;
    cmd_data->hci_cmd_buffer_length = 14;
}

void ble_aci_gap_forge_cmd_numeric_comparison_value_confirm_yesno(ble_cmd_data_t *cmd_data,
                                                                  uint16_t        connection_handle,
                                                                  uint8_t         confirm_yes_no)
{
    if (!cmd_data) {
        return;
    }

    start_packet_with_opcode(cmd_data, ACI_GAP_NUMERIC_COMPARISON_VALUE_CONFIRM_YESNO_CMD_CODE);
    U2LE_ENCODE(cmd_data->hci_cmd_buffer, 2, connection_handle);
    cmd_data->hci_cmd_buffer[4]     = confirm_yes_no;
    cmd_data->hci_cmd_buffer_length = 5;
}
void ble_aci_gap_forge_cmd_pass_key_resp(ble_cmd_data_t *cmd_data,
                                         uint16_t        connection_handle,
                                         uint32_t        pass_key)
{
    if (!cmd_data) {
        return;
    }

    start_packet_with_opcode(cmd_data, ACI_GAP_PASS_KEY_RESP_CMD_CODE);
    U2LE_ENCODE(cmd_data->hci_cmd_buffer, 2, connection_handle);
    U4LE_ENCODE(cmd_data->hci_cmd_buffer, 4, pass_key);
    cmd_data->hci_cmd_buffer_length = 8;
}

void ble_aci_gap_forge_cmd_clear_security_db(ble_cmd_data_t *cmd_data)
{
    if (!cmd_data) {
        return;
    }

    start_packet_with_opcode(cmd_data, ACI_GAP_CLEAR_SECURITY_DB_CMD_CODE);
}

void ble_aci_gap_forge_cmd_set_discoverable(ble_cmd_data_t                  *cmd_data,
                                            ble_cmd_set_discoverable_data_t *data)
{
    if ((!cmd_data) || (!data) || (data->local_name_length > BLE_GAP_MAX_LOCAL_NAME_LENGTH)) {
        return;
    }

    start_packet_with_opcode(cmd_data, ACI_GAP_SET_DISCOVERABLE_CMD_CODE);
    // Advertising_Type
    cmd_data->hci_cmd_buffer[cmd_data->hci_cmd_buffer_length++] = data->advertising_type;
    // Advertising_Interval_Min
    U2LE_ENCODE(
        cmd_data->hci_cmd_buffer, cmd_data->hci_cmd_buffer_length, data->advertising_interval_min);
    cmd_data->hci_cmd_buffer_length += 2;
    // Advertising_Interval_Max
    U2LE_ENCODE(
        cmd_data->hci_cmd_buffer, cmd_data->hci_cmd_buffer_length, data->advertising_interval_max);
    cmd_data->hci_cmd_buffer_length += 2;
    // Own_Address_Type
    cmd_data->hci_cmd_buffer[cmd_data->hci_cmd_buffer_length++] = data->own_address_type;
    // Advertising_Filter_Policy
    cmd_data->hci_cmd_buffer[cmd_data->hci_cmd_buffer_length++] = data->advertising_filter_policy;
    // Local_Name_Length
    cmd_data->hci_cmd_buffer[cmd_data->hci_cmd_buffer_length++] = data->local_name_length;
    // Local_Name
    if (data->local_name_length && data->local_name) {
        memcpy(&cmd_data->hci_cmd_buffer[cmd_data->hci_cmd_buffer_length],
               data->local_name,
               data->local_name_length);
    }
    cmd_data->hci_cmd_buffer_length += data->local_name_length;
    // Service_Uuid_length
    cmd_data->hci_cmd_buffer[cmd_data->hci_cmd_buffer_length++] = data->service_uuid_length;
    // Service_Uuid_List
    if (data->service_uuid_length && data->service_uuid_list) {
        memcpy(&cmd_data->hci_cmd_buffer[cmd_data->hci_cmd_buffer_length],
               data->service_uuid_list,
               data->service_uuid_length);
    }
    cmd_data->hci_cmd_buffer_length += data->service_uuid_length;
    // Slave_Conn_Interval_Min
    U2LE_ENCODE(
        cmd_data->hci_cmd_buffer, cmd_data->hci_cmd_buffer_length, data->slave_conn_interval_min);
    cmd_data->hci_cmd_buffer_length += 2;
    // Slave_Conn_Interval_Max
    U2LE_ENCODE(
        cmd_data->hci_cmd_buffer, cmd_data->hci_cmd_buffer_length, data->slave_conn_interval_max);
    cmd_data->hci_cmd_buffer_length += 2;
}

void ble_aci_gap_forge_cmd_set_non_discoverable(ble_cmd_data_t *cmd_data)
{
    if (!cmd_data) {
        return;
    }

    start_packet_with_opcode(cmd_data, ACI_GAP_SET_NON_DISCOVERABLE_CMD_CODE);
}

void ble_aci_gap_forge_cmd_update_adv_data(ble_cmd_data_t *cmd_data,
                                           uint8_t         adv_data_length,
                                           const uint8_t  *adv_data)
{
    if ((!cmd_data) || (!adv_data)
        || ((uint16_t) (adv_data_length + 3) > (uint16_t) sizeof(cmd_data->hci_cmd_buffer))) {
        return;
    }

    memset(&cmd_data->hci_cmd_buffer, 0, sizeof(cmd_data->hci_cmd_buffer));
    start_packet_with_opcode(cmd_data, ACI_GAP_UPDATE_ADV_DATA_CMD_CODE);
    cmd_data->hci_cmd_buffer[2] = adv_data_length;
    memcpy(&cmd_data->hci_cmd_buffer[3], adv_data, adv_data_length);
    cmd_data->hci_cmd_buffer_length = 3 + adv_data_length;
}

void ble_aci_gatt_forge_cmd_init(ble_cmd_data_t *cmd_data)
{
    if (!cmd_data) {
        return;
    }

    start_packet_with_opcode(cmd_data, ACI_GATT_INIT_CMD_CODE);
}

void ble_aci_l2cap_forge_cmd_connection_parameter_update(ble_cmd_data_t *cmd_data,
                                                         uint16_t        connection_handle,
                                                         uint16_t        min_conn_interval,
                                                         uint16_t        max_conn_interval,
                                                         uint16_t        conn_latency,
                                                         uint16_t        supervision_timeout)
{
    if (!cmd_data) {
        return;
    }

    start_packet_with_opcode(cmd_data, ACI_L2CAP_CONNECTION_PARAMETER_UPDATE_CMD_CODE);
    U2LE_ENCODE(cmd_data->hci_cmd_buffer, 2, connection_handle);
    U2LE_ENCODE(cmd_data->hci_cmd_buffer, 4, min_conn_interval);
    U2LE_ENCODE(cmd_data->hci_cmd_buffer, 6, max_conn_interval);
    U2LE_ENCODE(cmd_data->hci_cmd_buffer, 8, conn_latency);
    U2LE_ENCODE(cmd_data->hci_cmd_buffer, 10, supervision_timeout);
    cmd_data->hci_cmd_buffer_length = 12;
}

void ble_aci_gatt_forge_cmd_add_service(ble_cmd_data_t *cmd_data,
                                        uint8_t         service_uuid_type,
                                        const uint8_t  *service_uuid,
                                        uint8_t         service_type,
                                        uint8_t         max_attribute_records)
{
    if ((!cmd_data) || (!service_uuid)) {
        return;
    }

    start_packet_with_opcode(cmd_data, ACI_GATT_ADD_SERVICE_CMD_CODE);
    cmd_data->hci_cmd_buffer[cmd_data->hci_cmd_buffer_length++] = service_uuid_type;
    if (service_uuid_type == BLE_GATT_UUID_TYPE_128) {
        memcpy(&cmd_data->hci_cmd_buffer[cmd_data->hci_cmd_buffer_length], service_uuid, 16);
        cmd_data->hci_cmd_buffer_length += 16;
    }
    else {
        memcpy(&cmd_data->hci_cmd_buffer[cmd_data->hci_cmd_buffer_length], service_uuid, 2);
        cmd_data->hci_cmd_buffer_length += 2;
    }
    cmd_data->hci_cmd_buffer[cmd_data->hci_cmd_buffer_length++] = service_type;
    cmd_data->hci_cmd_buffer[cmd_data->hci_cmd_buffer_length++] = max_attribute_records;
}

void ble_aci_gatt_forge_cmd_add_char(ble_cmd_data_t *cmd_data, ble_cmd_add_char_data_t *data)
{
    if ((!cmd_data) || (!data) || (!data->char_uuid)) {
        return;
    }

    start_packet_with_opcode(cmd_data, ACI_GATT_ADD_CHAR_CMD_CODE);
    // Service_Handle
    U2LE_ENCODE(cmd_data->hci_cmd_buffer, cmd_data->hci_cmd_buffer_length, data->service_handle);
    cmd_data->hci_cmd_buffer_length += 2;
    // Char_UUID_Type
    cmd_data->hci_cmd_buffer[cmd_data->hci_cmd_buffer_length++] = data->char_uuid_type;
    // Char_UUID
    if (data->char_uuid_type == BLE_GATT_UUID_TYPE_128) {
        memcpy(&cmd_data->hci_cmd_buffer[cmd_data->hci_cmd_buffer_length], data->char_uuid, 16);
        cmd_data->hci_cmd_buffer_length += 16;
    }
    else {
        memcpy(&cmd_data->hci_cmd_buffer[cmd_data->hci_cmd_buffer_length], data->char_uuid, 2);
        cmd_data->hci_cmd_buffer_length += 2;
    }
    // Char_Value_Length
    U2LE_ENCODE(cmd_data->hci_cmd_buffer, cmd_data->hci_cmd_buffer_length, data->char_value_length);
    cmd_data->hci_cmd_buffer_length += 2;
    // Char_Properties
    cmd_data->hci_cmd_buffer[cmd_data->hci_cmd_buffer_length++] = data->char_properties;
    // Security_Permissions
    cmd_data->hci_cmd_buffer[cmd_data->hci_cmd_buffer_length++] = data->security_permissions;
    // GATT_Evt_Mask
    cmd_data->hci_cmd_buffer[cmd_data->hci_cmd_buffer_length++] = data->gatt_evt_mask;
    // Enc_Key_Size
    cmd_data->hci_cmd_buffer[cmd_data->hci_cmd_buffer_length++] = data->enc_key_size;
    // Is_Variable
    cmd_data->hci_cmd_buffer[cmd_data->hci_cmd_buffer_length++] = data->is_variable;
}

void ble_aci_gatt_forge_cmd_update_char_value(ble_cmd_data_t *cmd_data,
                                              uint16_t        service_handle,
                                              uint16_t        char_handle,
                                              uint8_t         val_offset,
                                              uint8_t         char_value_length,
                                              const uint8_t  *char_value)
{
    if ((!cmd_data)
        || ((uint16_t) (char_value_length + 8) > (uint16_t) sizeof(cmd_data->hci_cmd_buffer))) {
        return;
    }

    start_packet_with_opcode(cmd_data, ACI_GATT_UPDATE_CHAR_VALUE_CMD_CODE);
    U2LE_ENCODE(cmd_data->hci_cmd_buffer, 2, service_handle);
    U2LE_ENCODE(cmd_data->hci_cmd_buffer, 4, char_handle);
    cmd_data->hci_cmd_buffer[6] = val_offset;
    cmd_data->hci_cmd_buffer[7] = char_value_length;
    if (char_value_length && char_value) {
        memcpy(&cmd_data->hci_cmd_buffer[8], char_value, char_value_length);
    }
    cmd_data->hci_cmd_buffer_length = 8 + char_value_length;
}

void ble_aci_gatt_forge_cmd_write_resp(ble_cmd_data_t *cmd_data,
                                       uint16_t        connection_handle,
                                       uint16_t        attribute_handle,
                                       uint8_t         write_status,
                                       uint8_t         error_code,
                                       uint8_t         attribute_val_length,
                                       const uint8_t  *attribute_val)
{
    if ((!cmd_data)
        || ((uint16_t) (attribute_val_length + 9) > (uint16_t) sizeof(cmd_data->hci_cmd_buffer))) {
        return;
    }

    start_packet_with_opcode(cmd_data, ACI_GATT_WRITE_RESP_CMD_CODE);
    U2LE_ENCODE(cmd_data->hci_cmd_buffer, 2, connection_handle);
    U2LE_ENCODE(cmd_data->hci_cmd_buffer, 4, attribute_handle);
    cmd_data->hci_cmd_buffer[6] = write_status;
    cmd_data->hci_cmd_buffer[7] = error_code;
    cmd_data->hci_cmd_buffer[8] = attribute_val_length;
    if (attribute_val_length && attribute_val) {
        memcpy(&cmd_data->hci_cmd_buffer[9], attribute_val, attribute_val_length);
    }
    cmd_data->hci_cmd_buffer_length = 9 + attribute_val_length;
}

void ble_aci_gatt_forge_cmd_confirm_indication(ble_cmd_data_t *cmd_data, uint16_t connection_handle)
{
    if (!cmd_data) {
        return;
    }

    start_packet_with_opcode(cmd_data, ACI_GATT_CONFIRM_INDICATION_CMD_CODE);
    U2LE_ENCODE(cmd_data->hci_cmd_buffer, 2, connection_handle);
    cmd_data->hci_cmd_buffer_length = 4;
}

void ble_aci_gatt_forge_cmd_exchange_config(ble_cmd_data_t *cmd_data, uint16_t connection_handle)
{
    if (!cmd_data) {
        return;
    }

    start_packet_with_opcode(cmd_data, ACI_GATT_EXCHANGE_CONFIG_CMD_CODE);
    U2LE_ENCODE(cmd_data->hci_cmd_buffer, 2, connection_handle);
    cmd_data->hci_cmd_buffer_length = 4;
}
