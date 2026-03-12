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
#include "ble_cmd.h"
#include "ble_ledger_types.h"

/* Exported enumerations -----------------------------------------------------*/

/* Exported defines   --------------------------------------------------------*/

/* FIDO BLE Service UUID (16-bit): 0xFFFD
 * Per CTAP2 spec §8.3 - FIDO Alliance GATT service
 */
#define BLE_FIDO_SERVICE_UUID_16    (0xFFFD)

/* FIDO BLE Characteristic UUIDs (128-bit)
 * Per CTAP2 spec §8.3.1
 * Base: F1D0FFF1-DEAA-ECEE-B42F-C9BA7ED623BB
 */
#define BLE_FIDO_CHAR_UUID_CONTROL_POINT                                                     \
    {0xBB, 0x23, 0xD6, 0x7E, 0xBA, 0xC9, 0x2F, 0xB4,                                       \
     0xEE, 0xEC, 0xAA, 0xDE, 0xF1, 0xFF, 0xD0, 0xF1}

#define BLE_FIDO_CHAR_UUID_STATUS                                                            \
    {0xBB, 0x23, 0xD6, 0x7E, 0xBA, 0xC9, 0x2F, 0xB4,                                       \
     0xEE, 0xEC, 0xAA, 0xDE, 0xF2, 0xFF, 0xD0, 0xF1}

#define BLE_FIDO_CHAR_UUID_CONTROL_POINT_LENGTH                                              \
    {0xBB, 0x23, 0xD6, 0x7E, 0xBA, 0xC9, 0x2F, 0xB4,                                       \
     0xEE, 0xEC, 0xAA, 0xDE, 0xF3, 0xFF, 0xD0, 0xF1}

#define BLE_FIDO_CHAR_UUID_SERVICE_REVISION_BITFIELD                                         \
    {0xBB, 0x23, 0xD6, 0x7E, 0xBA, 0xC9, 0x2F, 0xB4,                                       \
     0xEE, 0xEC, 0xAA, 0xDE, 0xF4, 0xFF, 0xD0, 0xF1}

/* FIDO BLE framing command bytes (bit 7 set for init packets) */
#define FIDO_BLE_CMD_PING       (0x81)
#define FIDO_BLE_CMD_KEEPALIVE  (0x82)
#define FIDO_BLE_CMD_MSG        (0x83)
#define FIDO_BLE_CMD_CANCEL     (0xBE)
#define FIDO_BLE_CMD_ERROR      (0xBF)

/* FIDO Service Revision Bitfield (§8.3.6)
 * Bit 5: FIDO2 Rev 1
 * Bit 6: FIDO2 Rev 2
 * We support U2F 1.1 (bit 0) + U2F 1.2 (bit 1) + FIDO2 Rev 1 (bit 5) */
#define FIDO_BLE_SERVICE_REVISION_BITFIELD  (0x20)

/* Maximum number of GATT attribute records for the FIDO service
 * 1 service + 4 characteristics * 3 attributes each ~= 13, round up */
#define BLE_FIDO_MAX_SERVICE_RECORDS  (16U)

/* Exported types, structures, unions ----------------------------------------*/

/* Exported macros------------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/
extern const ble_profile_info_t BLE_LEDGER_PROFILE_u2f_info;

/* Exported functions prototypes--------------------------------------------- */
ble_profile_status_t BLE_LEDGER_PROFILE_u2f_init(ble_cmd_data_t *cmd_data, void *cookie);
ble_profile_status_t BLE_LEDGER_PROFILE_u2f_create_db(uint8_t  *hci_buffer,
                                                       uint16_t  length,
                                                       void     *cookie);
uint8_t BLE_LEDGER_PROFILE_u2f_handle_in_range(uint16_t gatt_handle, void *cookie);

void BLE_LEDGER_PROFILE_u2f_connection_evt(ble_connection_t *connection, void *cookie);
void BLE_LEDGER_PROFILE_u2f_connection_update_evt(ble_connection_t *connection, void *cookie);
void BLE_LEDGER_PROFILE_u2f_encryption_changed(bool encrypted, void *cookie);

ble_profile_status_t BLE_LEDGER_PROFILE_u2f_att_modified(uint8_t *hci_buffer,
                                                          uint16_t length,
                                                          void    *cookie);
ble_profile_status_t BLE_LEDGER_PROFILE_u2f_write_permit_req(uint8_t *hci_buffer,
                                                              uint16_t length,
                                                              void    *cookie);
ble_profile_status_t BLE_LEDGER_PROFILE_u2f_mtu_changed(uint16_t mtu, void *cookie);

ble_profile_status_t BLE_LEDGER_PROFILE_u2f_write_rsp_ack(void *cookie);
ble_profile_status_t BLE_LEDGER_PROFILE_u2f_update_char_value_ack(void *cookie);
bool                 BLE_LEDGER_PROFILE_u2f_is_busy(void *cookie);

ble_profile_status_t BLE_LEDGER_PROFILE_u2f_send_packet(const uint8_t *packet,
                                                         uint16_t       length,
                                                         void          *cookie);

int32_t BLE_LEDGER_PROFILE_u2f_data_ready(uint8_t *buffer, uint16_t max_length, void *cookie);

void BLE_LEDGER_PROFILE_u2f_setting(uint32_t id,
                                     uint8_t *buffer,
                                     uint16_t length,
                                     void    *cookie);
