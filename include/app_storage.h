/*****************************************************************************
 *   (c) 2024 Ledger SAS.
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

#include <stdint.h>
#include <stdbool.h>

#ifndef HAVE_BOLOS
/* "app_storage_data.h" needs to be created in all apps including this file */
#include "app_storage_data.h"
#endif  // HAVE_BOLOS

///< bit to indicate in properties that the application storage contains settings
#define APP_STORAGE_PROP_SETTINGS (1 << 0)
///< bit to indicate in properties that the application contains data
#define APP_STORAGE_PROP_DATA     (1 << 1)

/**
 * @brief Structure defining the header of application storage header
 *
 */
typedef struct app_storage_header_s {
    char     tag[4];          ///< ['N','V','R','A'] array, when properly initialized
    uint32_t size;            ///< size in bytes of the data (app_storage_data_t structure)
    uint16_t struct_version;  ///< version of the structure of data (to be set once at first
                              ///< application start-up)
    uint16_t properties;      ///< used as a bitfield to set properties, like: contains settings,
                              ///< contains sensitive data
    uint32_t data_version;    ///< version of the content of data (to be updated every time data are
                              ///< updated)
} app_storage_header_t;

/**
 * @brief Structure defining the application storage
 *
 */
typedef struct app_storage_s {
    app_storage_header_t header;  ///< header describing the data
#ifndef HAVE_BOLOS
    app_storage_data_t data;  ///< application data, app_storage_data_t must be defined in
                              ///< "app_storage_data.h" file
#endif                        // HAVE_BOLOS
} app_storage_t;

#ifndef HAVE_BOLOS
/**
 * @brief This variable must be defined in Application code, and never used directly,
 * except by @ref N_nvram
 *
 */
extern const app_storage_t N_app_storage_real;

/**
 * @brief To be used by function accessing application storage data (not N_storage_real)
 *
 */
#define N_app_storage (*(volatile app_storage_t *) PIC(&N_app_storage_real))

void     app_storage_init(uint32_t data_version);
uint32_t app_storage_get_size(void);
uint16_t app_storage_get_struct_version(void);
uint16_t app_storage_get_properties(void);
uint32_t app_storage_get_data_version(void);
bool     app_storage_is_initalized(void);
void     app_storage_set_data_version(uint32_t data_version);

#endif  // HAVE_BOLOS
