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
#include "u2f_types.h"

/* Exported enumerations -----------------------------------------------------*/
typedef enum {
    U2F_MEDIA_NONE,
    U2F_MEDIA_USB,
    U2F_MEDIA_NFC,
    U2F_MEDIA_BLE
} u2f_transport_media_t;

/* Exported types, structures, unions ----------------------------------------*/
typedef struct {
    u2f_transport_media_t media;
} u2f_service_t;

/* Exported defines   --------------------------------------------------------*/

/* Exported macros------------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/

/* Exported functions prototypes--------------------------------------------- */

// FIDO 2 compatible applications have to provide those implementations
void ctap2_handle_cmd_cbor(u2f_service_t *service, uint8_t *buffer, uint16_t length);
void ctap2_handle_cmd_cancel(u2f_service_t *service, uint8_t *buffer, uint16_t length);
