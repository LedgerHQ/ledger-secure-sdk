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
#include "os_types.h"
#include "os_io.h"

/* Exported enumerations -----------------------------------------------------*/
typedef enum {
    OS_IO_APDU_POST_ACTION_NONE,
    OS_IO_APDU_POST_ACTION_EXIT,
} os_io_apdu_post_action_t;

/* Exported types, structures, unions ----------------------------------------*/

/* Exported defines   --------------------------------------------------------*/
#define DEFAULT_APDU_CLA                   (0xB0)
#define DEFAULT_APDU_INS_GET_VERSION       (0x01)
#define DEFAULT_APDU_INS_GET_SEED_COOKIE   (0x02)
#define DEFAULT_APDU_INS_LOAD_CERTIFICATE  (0x06)
#define DEFAULT_APDU_INS_STACK_CONSUMPTION (0x57)
#define DEFAULT_APDU_INS_APP_EXIT          (0xA7)

/* Exported macros------------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/

/* Exported functions prototypes--------------------------------------------- */
bolos_err_t os_io_handle_default_apdu(uint8_t                  *buffer_in,
                                      size_t                    buffer_in_length,
                                      uint8_t                  *buffer_out,
                                      size_t                   *buffer_out_length,
                                      os_io_apdu_post_action_t *post_action);
