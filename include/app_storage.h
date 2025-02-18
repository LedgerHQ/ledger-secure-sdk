/*****************************************************************************
 *   (c) 2024, 2025 Ledger SAS.
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

///< bit to indicate in properties that the application storage contains settings
#define APP_STORAGE_PROP_SETTINGS (1 << 0)
///< bit to indicate in properties that the application contains data
#define APP_STORAGE_PROP_DATA     (1 << 1)

///< Tag length
#define APP_STORAGE_TAG_LEN 4

///< Tag value
#define APP_STORAGE_TAG "NVRA"

///<  Header structure version
#define APP_STORAGE_HEADER_STRUCT_VERSION 1

///< Error codes
#define APP_STORAGE_EINVAL              -1  ///< Invalid argument
#define APP_STORAGE_EADDRNOTAVAIL       -2  ///< Address not available for reading
#define APP_STORAGE_EOVERFLOW           -3  ///< Value too large to be stored

/// Initial app data storage version
#define APP_STORAGE_INITIAL_APP_DATA_VERSION 1

/**
 * @brief Structure defining the header of application storage header
 *
 */
typedef struct __attribute__((packed)) app_storage_header_s {
    char     tag[APP_STORAGE_TAG_LEN];  ///< ['N','V','R','A'] array, when properly initialized
    uint32_t size;            ///< size in bytes of the data (app_storage_data_t structure)
    uint16_t struct_version;  ///< version of this structure (for OS)
    uint16_t properties;      ///< used as a bitfield to set properties, like: contains settings,
                              ///< contains sensitive data
    uint32_t data_version;    ///< version of the content of data (to be updated every time data are
                              ///< updated)
} app_storage_header_t;

#ifndef HAVE_BOLOS

/* Getters for system information */
uint32_t app_storage_get_size(void);
uint16_t app_storage_get_properties(void);
uint32_t app_storage_get_data_version(void);

/* Reads app storage data */
int32_t app_storage_pread(void *buf, uint32_t nbyte, uint32_t offset);

/* Writes app storage data */
int32_t app_storage_pwrite(const void *buf, uint32_t nbyte, uint32_t offset);

/* Setters */
void app_storage_set_data_version(uint32_t data_version);
void app_storage_increment_data_version(void);

/* Resets and zeroes out application storage */
void app_storage_reset(void);

#endif  // #ifndef HAVE_BOLOS
