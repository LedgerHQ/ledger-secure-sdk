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

#ifndef USB_LEDGER_IAP_H
#define USB_LEDGER_IAP_H

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include "usbd_def.h"
#include "usbd_ledger_types.h"

/* Exported enumerations -----------------------------------------------------*/

/* Exported defines   --------------------------------------------------------*/

/* Exported types, structures, unions ----------------------------------------*/

/* Exported macros------------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/

void USB_LEDGER_IAP_init(void);

int USB_LEDGER_iap_rx_seph_evt(uint8_t *seph_buffer,
                               uint16_t seph_buffer_length,
                               uint8_t *apdu_buffer,
                               uint16_t apdu_buffer_max_length);

int USB_LEDGER_IAP_send_apdu(const uint8_t *apdu_buf, uint16_t apdu_buf_length);

#endif  // USBD_LEDGER_WEBUSB_H
