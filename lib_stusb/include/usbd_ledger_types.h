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

#ifndef USBD_LEDGER_TYPES_H
#define USBD_LEDGER_TYPES_H

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include "usbd_def.h"

/* Exported enumerations -----------------------------------------------------*/

/* Exported defines   --------------------------------------------------------*/
#define MAX_DESCRIPTOR_SIZE (256)

/* Exported types, structures, unions ----------------------------------------*/
typedef struct {
    uint8_t  ep_in_addr;
    uint16_t ep_in_size;
    uint8_t  ep_out_addr;
    uint16_t ep_out_size;
    uint8_t  ep_type;
} usbd_end_point_info_t;

typedef USBD_StatusTypeDef (*usbd_class_init_t)(USBD_HandleTypeDef *pdev, void *cookie);
typedef USBD_StatusTypeDef (*usbd_class_de_init_t)(USBD_HandleTypeDef *pdev, void *cookie);
typedef USBD_StatusTypeDef (*usbd_class_setup_t)(USBD_HandleTypeDef   *pdev,
                                      void                 *cookie,
                                      USBD_SetupReqTypedef *req);
typedef USBD_StatusTypeDef (*usbd_ep0_rx_ready_t)(USBD_HandleTypeDef *pdev, void *cookie);
typedef USBD_StatusTypeDef (*usbd_class_data_in_t)(USBD_HandleTypeDef *pdev, void *cookie, uint8_t ep_num);
typedef USBD_StatusTypeDef (*usbd_class_data_out_t)(USBD_HandleTypeDef *pdev,
                                         void               *cookie,
                                         uint8_t             ep_num,
                                         uint8_t            *packet,
                                         uint16_t            packet_length);

typedef USBD_StatusTypeDef (*usbd_send_packet_t)(USBD_HandleTypeDef *pdev,
                                      void               *cookie,
                                      uint8_t             packet_type,
                                      const uint8_t      *packet,
                                      uint16_t            packet_length,
                                      uint32_t            timeout_ms);
typedef bool (*usbd_is_busy_t)(void *cookie);

typedef int32_t (*usbd_data_ready_t)(USBD_HandleTypeDef *pdev,
                                     void               *cookie,
                                     uint8_t            *buffer,
                                     uint16_t            max_length);

typedef void (*usbd_setting_t)(uint32_t class_id, uint8_t *buffer, uint16_t length, void *cookie);

typedef struct usbd_class_info_t_ {
    uint8_t type;  // usbd_ledger_class_mask_e

    const usbd_end_point_info_t *end_point;

    usbd_class_init_t     init;
    usbd_class_de_init_t  de_init;
    usbd_class_setup_t    setup;
    usbd_ep0_rx_ready_t   ep0_rx_ready;
    usbd_class_data_in_t  data_in;
    usbd_class_data_out_t data_out;

    usbd_send_packet_t send_packet;
    usbd_is_busy_t     is_busy;

    usbd_data_ready_t data_ready;

    usbd_setting_t setting;

    uint8_t        interface_descriptor_size;
    const uint8_t *interface_descriptor;

    uint8_t        interface_association_descriptor_size;
    const uint8_t *interface_association_descriptor;

    uint8_t        bos_descriptor_size;
    const uint8_t *bos_descriptor;

    void *cookie;
} usbd_class_info_t;

/* Exported macros------------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/

/* Exported functions prototypes--------------------------------------------- */

#endif  // USBD_LEDGER_TYPES_H
