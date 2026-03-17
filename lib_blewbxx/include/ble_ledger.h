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
#include "os_id.h"
#include "lcx_crc.h"
#include "ble_types.h"

/* Exported enumerations -----------------------------------------------------*/
typedef enum {
    BLE_LEDGER_PROFILE_APDU = 0x0001,
    BLE_LEDGER_PROFILE_U2F  = 0x0004,
} ble_ledger_profile_mask_e;

/* Exported defines   --------------------------------------------------------*/
#define BLE_SLAVE_CONN_INTERVAL_MIN 12  // 15ms
#define BLE_SLAVE_CONN_INTERVAL_MAX 24  // 30ms

#define BLE_ADVERTISING_INTERVAL_MIN 48  // 30ms
#define BLE_ADVERTISING_INTERVAL_MAX 96  // 60ms

/* Exported types, structures, unions ----------------------------------------*/

/* Exported macros------------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/

/* Exported functions prototypes--------------------------------------------- */
void BLE_LEDGER_init(os_io_init_ble_t *init, uint8_t force);
void BLE_LEDGER_start(void);
void BLE_LEDGER_stop(void);

void BLE_LEDGER_reset_pairings(void);
void BLE_LEDGER_accept_pairing(uint8_t status);
void BLE_LEDGER_name_changed(void);

/**
 * @brief Derives the BLE MAC address from the product serial number.
 *
 * @param address_buf Pointer to a buffer of at least 6 bytes.
 * @param buf_len     Length of the provided buffer.
 */
void LEDGER_BLE_get_mac_address(uint8_t *address_buf, size_t buf_len);

// Rx
int BLE_LEDGER_rx_seph_evt(uint8_t *seph_buffer,
                           uint16_t seph_buffer_length,
                           uint8_t *apdu_buffer,
                           uint16_t apdu_buffer_max_length);

// Tx
uint32_t BLE_LEDGER_send(uint8_t        profile_type,
                         const uint8_t *packet,
                         uint16_t       packet_length,
                         uint32_t       timeout_ms);

// Check data sent
bool BLE_LEDGER_is_busy(void);

// Setting
void BLE_LEDGER_setting(uint32_t profile_id, uint32_t setting_id, uint8_t *buffer, uint16_t length);
