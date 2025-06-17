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

#ifndef USBD_DESC_H
#define USBD_DESC_H

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include "usbd_def.h"

/* Exported enumerations -----------------------------------------------------*/

/* Exported types, structures, unions ----------------------------------------*/

/* Exported defines   --------------------------------------------------------*/
#define USBD_LEDGER_VID (0x2C97)

/* Exported macros------------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/
extern const USBD_DescriptorsTypeDef LEDGER_Desc;

/* Exported functions prototypes--------------------------------------------- */
uint8_t *USBD_get_descriptor_device(USBD_SpeedTypeDef speed, uint16_t *length);
uint8_t *USBD_get_descriptor_lang_id(USBD_SpeedTypeDef speed, uint16_t *length);
uint8_t *USBD_get_descriptor_manufacturer(USBD_SpeedTypeDef speed, uint16_t *length);
uint8_t *USBD_get_descriptor_product(USBD_SpeedTypeDef speed, uint16_t *length);
uint8_t *USBD_get_descriptor_serial(USBD_SpeedTypeDef speed, uint16_t *length);
uint8_t *USBD_get_descriptor_BOS(USBD_SpeedTypeDef speed, uint16_t *length);

void USBD_DESC_init(char                  *product_str,
                    uint16_t               vid,
                    uint16_t               pid,
                    uint8_t                bcdusb,
                    uint8_t                iad,
                    USB_GetBOSDescriptor_t bos_descriptor);

#endif  // USBD_DESC_H
