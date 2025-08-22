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
#include "os_io.h"
#include "lcx_crc.h"
#include "ble_types.h"

#ifdef HAVE_ADVANCED_BLE_CMDS
#include "ble_ledger_types.h"
#endif  // HAVE_ADVANCED_BLE_CMDS
/* Exported enumerations -----------------------------------------------------*/
typedef enum {
    BLE_LEDGER_PROFILE_APDU = 0x0001,
    BLE_LEDGER_PROFILE_U2F  = 0x0004,
} ble_ledger_profile_mask_e;

/* Exported defines   --------------------------------------------------------*/
#define LEDGER_BLE_get_mac_address(address)            \
    {                                                  \
        unsigned char se_serial[8] = {0};              \
        os_serial(se_serial, sizeof(se_serial));       \
        unsigned int uid = cx_crc16(se_serial, 4);     \
        address[0]       = uid;                        \
        address[1]       = uid >> 8;                   \
        uid              = cx_crc16(se_serial + 4, 4); \
        address[2]       = uid;                        \
        address[3]       = uid >> 8;                   \
        address[4]       = 0xF1;                       \
        address[5]       = 0xDE;                       \
    }

#define BLE_SLAVE_CONN_INTERVAL_MIN 12  // 15ms
#define BLE_SLAVE_CONN_INTERVAL_MAX 24  // 30ms

#define BLE_ADVERTISING_INTERVAL_MIN 48  // 30ms
#define BLE_ADVERTISING_INTERVAL_MAX 96  // 60ms

/* Exported types, structures, unions ----------------------------------------*/
#ifdef HAVE_ADVANCED_BLE_CMDS
typedef void (*cdc_controller_packet_cb_t)(uint8_t *packet, uint16_t length);

typedef void (*cdc_event_cb_t)(uint32_t event, uint32_t param);

typedef struct ble_ledger_advanced_data_s {
    int8_t  rssi_level;
    int8_t  current_transmit_power_level;
    int8_t  max_transmit_power_level;
    int8_t  requested_tx_power;
    uint8_t hci_reading_current_tx_power;
    uint8_t adv_tx_power;
    uint8_t enabling_advertising;
    uint8_t disabling_advertising;
    uint8_t adv_enable;
} ble_ledger_advanced_data_t;

#endif  // HAVE_ADVANCED_BLE_CMDS
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

#ifdef HAVE_ADVANCED_BLE_CMDS
void    BLE_LEDGER_start_advanced(uint16_t profile_mask);
void    BLE_LEDGER_swith_to_bridge(void);
uint8_t BLE_LEDGER_enable_advertising(uint8_t enable);
void    BLE_LEDGER_start_bridge(cdc_controller_packet_cb_t cdc_controller_packet_cb,
                                cdc_event_cb_t             cdc_event_cb);
void    BLE_LEDGER_send_to_controller(uint8_t *packet, uint16_t packet_length);

uint8_t BLE_LEDGER_set_tx_power(uint8_t high_power, int8_t value);
int8_t  BLE_LEDGER_requested_tx_power(void);

uint8_t BLE_LEDGER_is_advertising_enabled(void);

uint8_t BLE_LEDGER_disconnect(void);

void BLE_LEDGER_confirm_numeric_comparison(uint8_t confirm);

void BLE_LEDGER_clear_pairings(void);

void BLE_LEDGER_get_connection_info(ble_connection_t           *connection_info,
                                    ble_ledger_advanced_data_t *advanced_data_info);

void BLE_LEDGER_trig_read_rssi(void);
void BLE_LEDGER_trig_read_transmit_power_level(uint8_t current);

uint8_t BLE_LEDGER_update_connection_interval(uint16_t conn_interval_min,
                                              uint16_t conn_interval_max);
#endif  // HAVE_ADVANCED_BLE_CMDS
