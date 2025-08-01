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

#ifndef USBD_LEDGER_HID_KBD_H
#define USBD_LEDGER_HID_KBD_H

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include "usbd_def.h"
#include "usbd_ledger_types.h"

/* Exported enumerations -----------------------------------------------------*/

/* Exported defines   --------------------------------------------------------*/

/* Exported types, structures, unions ----------------------------------------*/

/* Exported macros------------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/
extern const usbd_class_info_t USBD_LEDGER_HID_KBD_class_info;

/* Exported functions prototypes--------------------------------------------- */
USBD_StatusTypeDef USBD_LEDGER_HID_KBD_init(USBD_HandleTypeDef *pdev, void *cookie);
USBD_StatusTypeDef USBD_LEDGER_HID_KBD_de_init(USBD_HandleTypeDef *pdev, void *cookie);
USBD_StatusTypeDef USBD_LEDGER_HID_KBD_setup(USBD_HandleTypeDef   *pdev,
                                  void                 *cookie,
                                  USBD_SetupReqTypedef *req);
USBD_StatusTypeDef USBD_LEDGER_HID_KBD_ep0_rx_ready(USBD_HandleTypeDef *pdev, void *cookie);
USBD_StatusTypeDef USBD_LEDGER_HID_KBD_data_in(USBD_HandleTypeDef *pdev, void *cookie, uint8_t ep_num);
USBD_StatusTypeDef USBD_LEDGER_HID_KBD_data_out(USBD_HandleTypeDef *pdev,
                                     void               *cookie,
                                     uint8_t             ep_num,
                                     uint8_t            *packet,
                                     uint16_t            packet_length);

USBD_StatusTypeDef USBD_LEDGER_HID_KBD_send_packet(USBD_HandleTypeDef *pdev,
                                        void               *cookie,
                                        uint8_t             packet_type,
                                        const uint8_t      *packet,
                                        uint16_t            packet_length,
                                        uint32_t            timeout_ms);

#endif  // USBD_LEDGER_HID_KBD_H
