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
#include <string.h>

#include "os.h"
#include "os_helpers.h"
#include "lcx_crc.h"

/* Private enumerations ------------------------------------------------------*/

/* Private defines------------------------------------------------------------*/
// Flag to set the two most significant bits to 11 = random static address
#define BLE_ADDRESS_SET_MSB (0xC0)

/* Private types, structures, unions -----------------------------------------*/

/* Private macros-------------------------------------------------------------*/

/* Private functions prototypes ----------------------------------------------*/

/* Exported functions --------------------------------------------------------*/

void LEDGER_BLE_get_mac_address(uint8_t *address_buf, size_t buf_len)
{
    // Ensure we have enough space for a 6-byte MAC address
    if (address_buf == NULL || buf_len < 6) {
        return;
    }

    uint8_t  se_serial[8] = {0};
    uint16_t uid;

    os_serial(se_serial, sizeof(se_serial));

    uid            = cx_crc16(se_serial, 4);
    address_buf[0] = (uint8_t) (uid & 0xff);
    address_buf[1] = (uint8_t) (uid >> 8);
    uid            = cx_crc16(se_serial + 4, 4);
    address_buf[2] = (uint8_t) (uid & 0xff);
    address_buf[3] = (uint8_t) (uid >> 8);

    // Invert the serial number bits for the final derivation
    for (size_t i = 0; i < sizeof(se_serial); i++) {
        se_serial[i] = ~se_serial[i];
    }
    uid            = cx_crc16(se_serial, 8);
    address_buf[4] = (uint8_t) (uid & 0xff);
    // Set the two most significant bits for Random Static Address
    address_buf[5] = (uint8_t) ((uid >> 8) | BLE_ADDRESS_SET_MSB);
}
