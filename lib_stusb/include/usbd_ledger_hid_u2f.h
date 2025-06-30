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

#ifndef USBD_LEDGER_HID_U2F_H
#define USBD_LEDGER_HID_U2F_H

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include "usbd_def.h"
#include "usbd_ledger_types.h"

/* Exported enumerations -----------------------------------------------------*/
typedef enum {
    USBD_LEDGER_HID_U2F_SETTING_ID_VERSIONS          = 0,
    USBD_LEDGER_HID_U2F_SETTING_ID_CAPABILITIES_FLAG = 1,
    USBD_LEDGER_HID_U2F_SETTING_ID_FREE_CID          = 2,
} usdb_ledger_hid_u2f_setting_id_e;

/* Exported defines   --------------------------------------------------------*/

/* Exported types, structures, unions ----------------------------------------*/

/* Exported macros------------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/
extern const usbd_class_info_t USBD_LEDGER_HID_U2F_class_info;

/* Exported functions prototypes--------------------------------------------- */
USBD_StatusTypeDef USBD_LEDGER_HID_U2F_init(USBD_HandleTypeDef *pdev, void *cookie);
USBD_StatusTypeDef USBD_LEDGER_HID_U2F_de_init(USBD_HandleTypeDef *pdev, void *cookie);
USBD_StatusTypeDef USBD_LEDGER_HID_U2F_setup(USBD_HandleTypeDef   *pdev,
                                  void                 *cookie,
                                  USBD_SetupReqTypedef *req);
USBD_StatusTypeDef  USBD_LEDGER_HID_U2F_ep0_rx_ready(USBD_HandleTypeDef *pdev, void *cookie);
USBD_StatusTypeDef  USBD_LEDGER_HID_U2F_data_in(USBD_HandleTypeDef *pdev, void *cookie, uint8_t ep_num);
USBD_StatusTypeDef  USBD_LEDGER_HID_U2F_data_out(USBD_HandleTypeDef *pdev,
                                     void               *cookie,
                                     uint8_t             ep_num,
                                     uint8_t            *packet,
                                     uint16_t            packet_length);

USBD_StatusTypeDef  USBD_LEDGER_HID_U2F_send_message(USBD_HandleTypeDef *pdev,
                                         void               *cookie,
                                         uint8_t             packet_type,
                                         const uint8_t      *message,
                                         uint16_t            message_length,
                                         uint32_t            timeout_ms);
bool USBD_LEDGER_HID_U2F_is_busy(void *cookie);

int32_t USBD_LEDGER_HID_U2F_data_ready(USBD_HandleTypeDef *pdev,
                                       void               *cookie,
                                       uint8_t            *buffer,
                                       uint16_t            max_length);

void USBD_LEDGER_HID_U2F_setting(uint32_t id, uint8_t *buffer, uint16_t length, void *cookie);

#endif  // USBD_LEDGER_HID_U2F_H
