/* @BANNER@ */

#pragma once

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include "ble_cmd.h"
#include "ble_ledger_types.h"

/* Exported enumerations -----------------------------------------------------*/
typedef enum {
    BLE_LEDGER_PROFILE_APDU_SETTING_ID_TRANSFER_MODE = 1,
} ble_ledger_profile_apdu_setting_id_e;

/* Exported defines   --------------------------------------------------------*/

/* Exported types, structures, unions ----------------------------------------*/

/* Exported macros------------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/
extern const ble_profile_info_t BLE_LEDGER_PROFILE_apdu_info;

/* Exported functions prototypes--------------------------------------------- */
void    BLE_LEDGER_PROFILE_apdu_init(ble_cmd_data_t *cmd_data, void *cookie);
uint8_t BLE_LEDGER_PROFILE_apdu_create_db(uint8_t *hci_buffer, uint16_t length, void *cookie);
uint8_t BLE_LEDGER_PROFILE_apdu_handle_in_range(uint16_t gatt_handle, void *cookie);

void BLE_LEDGER_PROFILE_apdu_connection_evt(ble_connection_t *connection, void *cookie);
void BLE_LEDGER_PROFILE_apdu_connection_update_evt(ble_connection_t *connection, void *cookie);
void BLE_LEDGER_PROFILE_apdu_encryption_changed(uint8_t encrypted, void *cookie);

uint8_t BLE_LEDGER_PROFILE_apdu_att_modified(uint8_t *hci_buffer, uint16_t length, void *cookie);
uint8_t BLE_LEDGER_PROFILE_apdu_write_permit_req(uint8_t *hci_buffer,
                                                 uint16_t length,
                                                 void    *cookie);
uint8_t BLE_LEDGER_PROFILE_apdu_mtu_changed(uint16_t mtu, void *cookie);

uint8_t BLE_LEDGER_PROFILE_apdu_write_rsp_ack(void *cookie);
uint8_t BLE_LEDGER_PROFILE_apdu_update_char_value_ack(void *cookie);

uint8_t BLE_LEDGER_PROFILE_apdu_send_packet(const uint8_t *packet, uint16_t length, void *cookie);

int32_t BLE_LEDGER_PROFILE_apdu_data_ready(uint8_t *buffer, uint16_t max_length, void *cookie);

void BLE_LEDGER_PROFILE_apdu_setting(uint32_t id, uint8_t *buffer, uint16_t length, void *cookie);