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

#ifndef USBD_LEDGER_H
#define USBD_LEDGER_H

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include "os_io.h"

#include "usbd_ledger_types.h"

/* Exported enumerations -----------------------------------------------------*/
typedef enum {
    USBD_LEDGER_PRODUCT_BLUE       = 0x0000,
    USBD_LEDGER_PRODUCT_NANOS      = 0x1000,
    USBD_LEDGER_PRODUCT_HW2        = 0x3000,
    USBD_LEDGER_PRODUCT_NANOX      = 0x4000,
    USBD_LEDGER_PRODUCT_NANOS_PLUS = 0x5000,
    USBD_LEDGER_PRODUCT_STAX       = 0x6000,
    USBD_LEDGER_PRODUCT_FLEX       = 0x7000,
    USBD_LEDGER_PRODUCT_APEX_P     = 0x8000,
    USBD_LEDGER_PRODUCT_APEX_M     = 0x9000,
} usbd_ledger_product_e;

typedef enum {
    USBD_LEDGER_INVERTED_PRODUCT_BLUE       = 0x0000,
    USBD_LEDGER_INVERTED_PRODUCT_NANOS      = 0x0001,
    USBD_LEDGER_INVERTED_PRODUCT_HW2        = 0x0003,
    USBD_LEDGER_INVERTED_PRODUCT_NANOX      = 0x0004,
    USBD_LEDGER_INVERTED_PRODUCT_NANOS_PLUS = 0x0005,
    USBD_LEDGER_INVERTED_PRODUCT_STAX       = 0x0006,
    USBD_LEDGER_INVERTED_PRODUCT_FLEX       = 0x0007,
    USBD_LEDGER_INVERTED_PRODUCT_APEX_P     = 0x0008,
    USBD_LEDGER_INVERTED_PRODUCT_APEX_M     = 0x0009,
} usbd_ledger_inverted_product_e;

typedef enum {
    USBD_LEDGER_CLASS_HID         = 0x0001,
    USBD_LEDGER_CLASS_HID_KBD     = 0x0002,
    USBD_LEDGER_CLASS_HID_U2F     = 0x0004,
    USBD_LEDGER_CLASS_CCID_BULK   = 0x0008,
    USBD_LEDGER_CLASS_WEBUSB      = 0x0010,
    USBD_LEDGER_CLASS_CDC_CONTROL = 0x0020,
    USBD_LEDGER_CLASS_CDC_DATA    = 0x0040,
    USBD_LEDGER_CLASS_CDC         = USBD_LEDGER_CLASS_CDC_CONTROL | USBD_LEDGER_CLASS_CDC_DATA,
} usbd_ledger_class_mask_e;

/* Exported defines   --------------------------------------------------------*/
#define LEDGER_USBD_DEFAULT_EPOUT_SIZE (0x40)

/* Exported types, structures, unions ----------------------------------------*/

/* Exported macros------------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/
extern uint8_t USBD_LEDGER_io_buffer[OS_IO_BUFFER_SIZE + 1];
extern uint8_t USBD_LEDGER_protocol_chunk_buffer[LEDGER_USBD_DEFAULT_EPOUT_SIZE];

/* Exported functions prototypes--------------------------------------------- */
void USBD_LEDGER_init(os_io_init_usb_t *init, uint8_t force_restart);
void USBD_LEDGER_start(void);
void USB_LEDGER_stop(void);

void USBD_LEDGER_add_profile(const usbd_class_info_t *class_info, uint8_t bcdusb, uint8_t usbd_iad);

// Rx
int USBD_LEDGER_rx_seph_evt(uint8_t *seph_buffer,
                            uint16_t seph_buffer_length,
                            uint8_t *apdu_buffer,
                            uint16_t apdu_buffer_max_length);

void USBD_LEDGER_rx_evt_reset(void);
void USBD_LEDGER_rx_evt_sof(void);
void USBD_LEDGER_rx_evt_suspend(void);
void USBD_LEDGER_rx_evt_resume(void);
void USBD_LEDGER_rx_evt_setup(uint8_t *buffer);
void USBD_LEDGER_rx_evt_data_in(uint8_t ep_num, uint8_t *buffer);
void USBD_LEDGER_rx_evt_data_out(uint8_t ep_num, uint8_t *buffer, uint16_t length);

// Tx
uint32_t USBD_LEDGER_send(uint8_t        class_type,
                          uint8_t        packet_type,
                          const uint8_t *packet,
                          uint16_t       packet_length,
                          uint32_t       timeout_ms);

// Check data received
int32_t USBD_LEDGER_data_ready(uint8_t *buffer, uint16_t max_length);

// Check data sent
int32_t USBD_LEDGER_is_busy(void);

// Setting
void USBD_LEDGER_setting(uint32_t class_id, uint32_t setting_id, uint8_t *buffer, uint16_t length);

#endif  // USBD_LEDGER_H
