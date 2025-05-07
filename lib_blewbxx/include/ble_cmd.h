/* @BANNER@ */

#pragma once

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include "ble_types.h"

/* Exported enumerations -----------------------------------------------------*/

/* Exported types, structures, unions ----------------------------------------*/
typedef struct {
    uint16_t hci_cmd_opcode;
    uint8_t  hci_cmd_buffer[BLE_ATT_MAX_MTU_SIZE+16];
    uint16_t hci_cmd_buffer_length;
} ble_cmd_data_t;

/* Exported defines   --------------------------------------------------------*/

/* Exported macros------------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/

/* Exported functions prototypes--------------------------------------------- */

/* HCI cmd */
void ble_hci_forge_cmd_reset(ble_cmd_data_t *cmd_data);
void ble_hci_forge_cmd_disconnect(ble_cmd_data_t *cmd_data, uint16_t connection_handle);
void ble_hci_forge_cmd_set_scan_response_data(ble_cmd_data_t *cmd_data,
                                              uint8_t         scan_response_data_length,
                                              const uint8_t  *scan_response_data);

/* ACI HAL cmd */
void ble_aci_hal_forge_cmd_write_config_data(ble_cmd_data_t *cmd_data,
                                             uint8_t         offset,
                                             uint8_t         length,
                                             const uint8_t  *value);
void ble_aci_hal_forge_cmd_set_tx_power_level(ble_cmd_data_t *cmd_data,
                                              uint8_t         enable_high_power,
                                              uint8_t         pa_level);

/* ACI GAP cmd */
void ble_aci_gap_forge_cmd_init(ble_cmd_data_t *cmd_data,
                                uint8_t         role,
                                uint8_t         privacy_enabled,
                                uint8_t         device_name_char_len);
void ble_aci_gap_forge_cmd_set_io_capability(ble_cmd_data_t *cmd_data, uint8_t io_capability);
void ble_aci_gap_forge_cmd_set_authentication_requirement(ble_cmd_data_t              *cmd_data,
                                                          ble_cmd_set_auth_req_data_t *data);
void ble_aci_gap_forge_cmd_numeric_comparison_value_confirm_yesno(ble_cmd_data_t *cmd_data,
                                                                  uint16_t        connection_handle,
                                                                  uint8_t         confirm_yes_no);
void ble_aci_gap_forge_cmd_pass_key_resp(ble_cmd_data_t *cmd_data,
                                         uint16_t        connection_handle,
                                         uint32_t        pass_key);
void ble_aci_gap_forge_cmd_clear_security_db(ble_cmd_data_t *cmd_data);
void ble_aci_gap_forge_cmd_set_discoverable(ble_cmd_data_t                  *cmd_data,
                                            ble_cmd_set_discoverable_data_t *data);
void ble_aci_gap_forge_cmd_set_non_discoverable(ble_cmd_data_t *cmd_data);
void ble_aci_gap_forge_cmd_update_adv_data(ble_cmd_data_t *cmd_data,
                                           uint8_t         adv_data_length,
                                           const uint8_t  *adv_data);

/* ACI L2CAP cmd */
void ble_aci_l2cap_forge_cmd_connection_parameter_update(ble_cmd_data_t *cmd_data,
                                                         uint16_t        connection_handle,
                                                         uint16_t        min_conn_interval,
                                                         uint16_t        max_conn_interval,
                                                         uint16_t        conn_latency,
                                                         uint16_t        supervision_timeout);

/* ACI GATT cmd */
void ble_aci_gatt_forge_cmd_init(ble_cmd_data_t *cmd_data);
void ble_aci_gatt_forge_cmd_add_service(ble_cmd_data_t *cmd_data,
                                        uint8_t         service_uuid_type,
                                        const uint8_t  *service_uuid,
                                        uint8_t         service_type,
                                        uint8_t         max_attribute_records);
void ble_aci_gatt_forge_cmd_add_char(ble_cmd_data_t *cmd_data, ble_cmd_add_char_data_t *data);
void ble_aci_gatt_forge_cmd_update_char_value(ble_cmd_data_t *cmd_data,
                                              uint16_t        service_handle,
                                              uint16_t        char_handle,
                                              uint8_t         val_offset,
                                              uint8_t         char_value_length,
                                              const uint8_t  *char_value);
void ble_aci_gatt_forge_cmd_write_resp(ble_cmd_data_t *cmd_data,
                                       uint16_t        connection_handle,
                                       uint16_t        attribute_handle,
                                       uint8_t         write_status,
                                       uint8_t         error_code,
                                       uint8_t         attribute_val_length,
                                       const uint8_t  *attribute_val);
void ble_aci_gatt_forge_cmd_confirm_indication(ble_cmd_data_t *cmd_data,
                                               uint16_t        connection_handle);
void ble_aci_gatt_forge_cmd_exchange_config(ble_cmd_data_t *cmd_data, uint16_t connection_handle);

/*
aci_gap_clear_security_db();
aci_gap_pass_key_resp
*/
