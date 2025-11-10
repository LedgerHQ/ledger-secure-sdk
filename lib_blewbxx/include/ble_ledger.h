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

#define BLE_ADDRESS_SET_MSB (0xC0)

/* Exported defines   --------------------------------------------------------*/
// Random static address is specified at
// https://wiki.st.com/stm32mcu/wiki/Connectivity:STM32WB-WBA_BLE_Privacy#Random_static_address
// In particular, the two most significant bits need to be set to 1.
#define LEDGER_BLE_get_mac_address(address)                    \
    {                                                          \
        unsigned char se_serial[8] = {0};                      \
        os_serial(se_serial, sizeof(se_serial));               \
        unsigned int uid = cx_crc16(se_serial, 4);             \
        address[0]       = uid;                                \
        address[1]       = uid >> 8;                           \
        uid              = cx_crc16(se_serial + 4, 4);         \
        address[2]       = uid;                                \
        address[3]       = uid >> 8;                           \
        for (unsigned int i = 0; i < sizeof(se_serial); i++) { \
            se_serial[i] = ~se_serial[i];                      \
        }                                                      \
        uid        = cx_crc16(se_serial, 8);                   \
        address[4] = uid;                                      \
        address[5] = (uid >> 8) | BLE_ADDRESS_SET_MSB;         \
    }

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
