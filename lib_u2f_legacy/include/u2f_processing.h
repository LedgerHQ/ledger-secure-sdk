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
#include <stdbool.h>
#include "u2f_service.h"

/* Exported enumerations -----------------------------------------------------*/

/* Exported types, structures, unions ----------------------------------------*/

/* Exported defines   --------------------------------------------------------*/

/* Exported macros------------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/

/* Exported functions prototypes--------------------------------------------- */
void u2f_message_set_autoreply_wait_user_presence(u2f_service_t *service, bool enabled);
void u2f_message_reply(u2f_service_t *service, uint8_t cmd, uint8_t *buffer, uint16_t len);
void u2f_transport_ctap2_send_keepalive(u2f_service_t *service, uint8_t reason);
