/* @BANNER@ */

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
        address[4]       = 0xF3;                       \
        address[5]       = 0xDE;                       \
    }

/* Exported types, structures, unions ----------------------------------------*/

/* Exported macros------------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/
extern uint8_t BLE_LEDGER_apdu_buffer[IO_APDU_BUFFER_SIZE];

/* Exported functions prototypes--------------------------------------------- */
void BLE_LEDGER_init(void);
void BLE_LEDGER_start(uint16_t profile_mask);  // mask forged with ble_ledger_profile_mask_e
void BLE_LEDGER_enable_advertising(uint8_t enable);
void BLE_LEDGER_reset_pairings(void);

// Rx
int BLE_LEDGER_rx_seph_evt(uint8_t *seph_buffer,
                           uint16_t seph_buffer_length,
                           uint8_t *apdu_buffer,
                           uint16_t apdu_buffer_max_length);

// Tx
uint32_t BLE_LEDGER_send(uint8_t *packet, uint16_t packet_length, uint32_t timeout_ms);

// Check APDU
int32_t BLE_LEDGER_data_ready(uint8_t *buffer, uint16_t max_length);
